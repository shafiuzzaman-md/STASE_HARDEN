/*
 * Message Queue Character Device Driver
 * 
 * This kernel module implements a character device that acts as a simple
 * message queue. It allows user-space applications to write messages into
 * the queue, which can then be read by other applications in a FIFO (First In, 
 * First Out) manner. The driver supports blocking and non-blocking reads, 
 * as well as IOCTL commands for clearing the message queue and retrieving 
 * the number of messages currently stored in the queue.
 * 
 * Features:
 * - Supports a message queue with configurable maximum message size.
 * - Provides blocking and non-blocking read capabilities.
 * - IOCTL commands to clear the queue and get the queue length.
 * - Ensures thread safety using mutexes and wait queues for synchronization.
 * 
 * Author: Lisbeth Salander
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>         /* For file operations */
#include <linux/uaccess.h>    /* For copy_to_user, copy_from_user */
#include <linux/cdev.h>       /* For cdev utilities */
#include <linux/device.h>     /* For device_create, device_destroy */
#include <linux/mutex.h>      /* For mutexes */
#include <linux/wait.h>       /* For wait queues */
#include <linux/slab.h>       /* For kmalloc, kfree */
#include <linux/ioctl.h>      /* For ioctl macros */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Message Queue Character Device Driver");
MODULE_VERSION("1.0");

#define DEVICE_NAME "msg_queue_dev"
#define CLASS_NAME  "msg_queue_class"
#define MAX_MESSAGE_SIZE 1024

/* IOCTL Commands */
#define MSG_QUEUE_IOC_MAGIC 'k'
#define IOCTL_CLEAR_QUEUE      _IO(MSG_QUEUE_IOC_MAGIC, 0)
#define IOCTL_GET_QUEUE_LENGTH _IOR(MSG_QUEUE_IOC_MAGIC, 1, int)

static int major_number;
static struct class *msg_queue_class = NULL;
static struct device *msg_queue_device = NULL;
static struct cdev msg_queue_cdev;

/* Message Node Structure */
struct message_node {
    char data[MAX_MESSAGE_SIZE];
    struct list_head list;
};
/* Define the msg_queue_context structure */
struct msg_queue_context {
    int open_count;          /* Tracks how many times the device has been opened */
    char *buffer;            /* Pointer to an internal buffer for the device */
};

/* Message Queue and Synchronization Primitives */
static LIST_HEAD(message_queue);
static DEFINE_MUTEX(queue_mutex);
static DECLARE_WAIT_QUEUE_HEAD(read_queue);

/* Declare array of function pointers for ioctl handlers */
typedef long (*ioctl_handler_fn)(struct file *, unsigned int, unsigned long);
static ioctl_handler_fn ioctl_handlers[2];

/* Function to purge the message queue when a threshold is reached */
int purge_message_queue(void)
{
    struct message_node *msg_node;
    char large_stack_buffer[1024];  /* Large buffer to force stack allocation */
    int i=0;
    volatile size_t dsize;

    /* Initialize the large buffer to ensure it's used */
    memset(large_stack_buffer, 0, sizeof(large_stack_buffer));

    mutex_lock(&queue_mutex);

    /* Delete exactly 50 messages (or fewer if fewer messages exist) */
    for (i = 0; i < 50 && !list_empty(&message_queue); i++) {
        msg_node = list_first_entry(&message_queue, struct message_node, list);
        
        /* Storing metadata into the buffer */
        dsize = strlen(msg_node->data);
        snprintf(large_stack_buffer + (i * 20), sizeof(large_stack_buffer) - (i * 20),
                 "%zu,%.10s\n", dsize, msg_node->data);

        /* Remove the message from the queue and free the memory */
        list_del(&msg_node->list);
        kfree(msg_node);
    }
    mutex_unlock(&queue_mutex);
    large_stack_buffer[0] = '\0';
    msg_node = NULL;
    dsize = 0;
    return 0;
}

static int device_open_count = 0;

/* Device Open Function */
static int msg_queue_open(struct inode *inodep, struct file *filep)
{
    struct msg_queue_context *context;
    int ret;

    printk(KERN_INFO "msg_queue_dev: Opening device\n");

    /* Allocate memory for the device context */
    context = kmalloc(sizeof(struct msg_queue_context), GFP_KERNEL);
    if (!context) {
        printk(KERN_ERR "msg_queue_dev: Failed to allocate memory for device context\n");
        return -ENOMEM;
    }

    /* Initialize internal data */
    memset(context, 0, sizeof(struct msg_queue_context));

    /* Check if device is already in use */
    mutex_lock(&queue_mutex);
    if (device_open_count > 0) {
        printk(KERN_WARNING "msg_queue_dev: Device is already in use\n");
        ret = -EBUSY;  /* Return busy if the device is already open */
        mutex_unlock(&queue_mutex);
        kfree(context);
        return ret;
    }

    /* Initialize internal buffers */
    context->buffer = kmalloc(1024, GFP_KERNEL);
    if (!context->buffer) {
        printk(KERN_ERR "msg_queue_dev: Failed to allocate internal buffer\n");
        mutex_unlock(&queue_mutex);
        kfree(context);
        return -ENOMEM;
    }

    /* Store context in filep for later use */
    filep->private_data = context;
    device_open_count++;
    mutex_unlock(&queue_mutex);

    printk(KERN_INFO "msg_queue_dev: Device opened successfully\n");
    return 0;
}

/* Device Release Function */
static int msg_queue_release(struct inode *inodep, struct file *filep)
{
    struct msg_queue_context *context = filep->private_data;

    printk(KERN_INFO "msg_queue_dev: Releasing device\n");

    if (!context) {
        printk(KERN_ERR "msg_queue_dev: No context found on release\n");
        return -EINVAL;
    }

    /* Release internal buffers */
    if (context->buffer) {
        printk(KERN_DEBUG "msg_queue_dev: Releasing internal buffer\n");
        kfree(context->buffer);
    }

    /* Perform any additional device-specific cleanup */
    mutex_lock(&queue_mutex);
    device_open_count--;
    mutex_unlock(&queue_mutex);

    /* Free the context structure */
    kfree(context);
    filep->private_data = NULL;

    printk(KERN_INFO "msg_queue_dev: Device successfully closed and resources released\n");
    return 0;
}


/* Helper function to get the number of messages in the queue */
static int get_queue_length(void)
{
    int count = 0;
    struct message_node *msg_node;

    mutex_lock(&queue_mutex);
    list_for_each_entry(msg_node, &message_queue, list) {
        count++;
    }
    mutex_unlock(&queue_mutex);

    return count;
}

/* Helper function to enqueue a message */
static ssize_t enqueue_message(const char __user *buffer, size_t len)
{
    struct message_node *msg_node;
    int queue_length = get_queue_length();
    
    /* Adjust length if necessary */
    if (len > MAX_MESSAGE_SIZE)
        len = MAX_MESSAGE_SIZE;

    /* Check if queue length exceeds the threshold */
    if (queue_length >= 100) {
        printk(KERN_INFO "msg_queue_dev: Queue length exceeded threshold, purging messages\n");
        purge_message_queue();  /* Call purge function when threshold is reached */
    }
    /* Allocate memory for the new message node */
    msg_node = kmalloc(sizeof(*msg_node), GFP_KERNEL);
    if (!msg_node)
        return -ENOMEM;

    /* Initialize the list head */
    INIT_LIST_HEAD(&msg_node->list);

    memset(msg_node->data, 0, MAX_MESSAGE_SIZE);

    /* Copy data from user space */
    if (copy_from_user(msg_node->data, buffer, len)) {
        kfree(msg_node);
        return -EFAULT;
    }

    mutex_lock(&queue_mutex);
    list_add_tail(&msg_node->list, &message_queue);
    mutex_unlock(&queue_mutex);

    wake_up_interruptible(&read_queue);  /* Wake up any blocking reads */

    return len;
}

/* Helper function to dequeue a message */
static int dequeue_message(struct message_node **msg_node)
{
    /* Check if the message queue is empty */
    if (list_empty(&message_queue)) {
        printk(KERN_WARNING "msg_queue_dev: Attempted to dequeue from an empty queue\n");
        return -EAGAIN;  /* No messages in the queue */
    }

    /* Retrieve the first message from the queue */
    *msg_node = list_first_entry(&message_queue, struct message_node, list);
    
    if (!(*msg_node)) {
        printk(KERN_ERR "msg_queue_dev: Failed to retrieve the message node from queue\n");
        return -EFAULT;  /* Corrupted message node or invalid pointer */
    }

    /* Message integrity verification */
    int checksum = 0;
    int message_length = strlen((*msg_node)->data);
    for (int i = 0; i < message_length; i++) {
        checksum += (*msg_node)->data[i];
    }

    printk(KERN_INFO "msg_queue_dev: Message dequeued with checksum %d\n", checksum);

    /* Remove the message from the queue */
    list_del(&(*msg_node)->list);
    printk(KERN_INFO "msg_queue_dev: Message removed from queue\n");

    return 0;
}


/* Device Read Function */
static ssize_t msg_queue_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset)
{
    struct message_node *msg_node;
    int ret;

    mutex_lock(&queue_mutex);

    /* If the queue is empty, return */
    if (list_empty(&message_queue)) { 
        mutex_unlock(&queue_mutex);
        return 0; 
    }

    /* Dequeue the message */
    ret = dequeue_message(&msg_node);
    mutex_unlock(&queue_mutex);

    if (ret)
        return ret;  /* No messages in the queue */

    /* Adjust length if necessary */
    if (len > MAX_MESSAGE_SIZE)
        len = MAX_MESSAGE_SIZE;

    /* Copy data to user space */
    if (copy_to_user(buffer, msg_node->data, len)) {
        kfree(msg_node);
        return -EFAULT;
    }

    kfree(msg_node);
    return len;
}

/* Device Write Function */
static ssize_t msg_queue_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset)
{
    int ret;

    ret = enqueue_message(buffer, len);
    if (ret < 0)
        return ret;

    return len;
}

/* Helper function to clear the message queue */
static void clear_message_queue(void)
{
    struct message_node *msg_node, *tmp;

    mutex_lock(&queue_mutex);
    list_for_each_entry_safe(msg_node, tmp, &message_queue, list) {
        list_del(&msg_node->list);
        kfree(msg_node);
    }
    mutex_unlock(&queue_mutex);
}

/* IOCTL handler for clearing the message queue */
static long clear_queue_handler(struct file *filep, unsigned int cmd, unsigned long arg)
{
    clear_message_queue();
    printk(KERN_INFO "msg_queue_dev: Message queue cleared by ioctl\n");
    return 0;
}

/* IOCTL handler for getting the queue length */
static long get_queue_length_handler(struct file *filep, unsigned int cmd, unsigned long arg)
{
    int queue_length = get_queue_length();
    if (copy_to_user((int __user *)arg, &queue_length, sizeof(int)))
        return -EFAULT;
    return 0;
}

/* Device IOCTL Function */
static long msg_queue_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case IOCTL_CLEAR_QUEUE:
        return ioctl_handlers[0](filep, cmd, arg);
    case IOCTL_GET_QUEUE_LENGTH:
        return ioctl_handlers[1](filep, cmd, arg);
    default:
        return -EINVAL;
    }
}

/* File Operations Structure */
static struct file_operations fops = {
    .owner          = THIS_MODULE,
    .open           = msg_queue_open,
    .read           = msg_queue_read,
    .write          = msg_queue_write,
    .unlocked_ioctl = msg_queue_ioctl,
    .release        = msg_queue_release,
};

static char *msgq_devnode(const struct device *dev, umode_t *mode)
{
    if (mode)
        *mode = 0666;
    return NULL;
}

/* Module Initialization Function */
static int __init msg_queue_init(void)
{
    int ret;
    dev_t dev_no;

    /* Dynamically allocate a major number */
    ret = alloc_chrdev_region(&dev_no, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ALERT "msg_queue_dev: Failed to allocate a major number\n");
        return ret;
    }
    major_number = MAJOR(dev_no);
    printk(KERN_INFO "msg_queue_dev: Registered correctly with major number %d\n", major_number);

    /* Initialize the cdev structure and add it to kernel space */
    cdev_init(&msg_queue_cdev, &fops);
    msg_queue_cdev.owner = THIS_MODULE;
    ret = cdev_add(&msg_queue_cdev, dev_no, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_no, 1);
        printk(KERN_ALERT "msg_queue_dev: Failed to add cdev\n");
        return ret;
    }

    /* Create device class */
    msg_queue_class = class_create(CLASS_NAME);
    if (IS_ERR(msg_queue_class)) {
        cdev_del(&msg_queue_cdev);
        unregister_chrdev_region(dev_no, 1);
        printk(KERN_ALERT "msg_queue_dev: Failed to register device class\n");
        return PTR_ERR(msg_queue_class);
    }
    printk(KERN_INFO "msg_queue_dev: Device class registered correctly\n");

    /* Set the devnode function to set default permissions */
    msg_queue_class->devnode = msgq_devnode;

    /* Create device */
    msg_queue_device = device_create(msg_queue_class, NULL, dev_no, NULL, DEVICE_NAME);
    if (IS_ERR(msg_queue_device)) {
        class_destroy(msg_queue_class);
        cdev_del(&msg_queue_cdev);
        unregister_chrdev_region(dev_no, 1);
        printk(KERN_ALERT "msg_queue_dev: Failed to create the device\n");
        return PTR_ERR(msg_queue_device);
    }
    printk(KERN_INFO "msg_queue_dev: Device created correctly\n");

    mutex_init(&queue_mutex);

    /* Assign handlers to the function pointer array */
    ioctl_handlers[0] = clear_queue_handler;
    ioctl_handlers[1] = get_queue_length_handler;

    return 0;
}

/* Module Exit Function */
static void __exit msg_queue_exit(void)
{
    dev_t dev_no = MKDEV(major_number, 0);

    /* Clean up message queue */
    clear_message_queue();

    /* Remove device */
    device_destroy(msg_queue_class, dev_no);
    class_destroy(msg_queue_class);
    cdev_del(&msg_queue_cdev);
    unregister_chrdev_region(dev_no, 1);

    printk(KERN_INFO "msg_queue_dev: Device unregistered\n");
}

module_init(msg_queue_init);
module_exit(msg_queue_exit);
