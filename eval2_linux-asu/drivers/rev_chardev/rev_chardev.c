/*
 * Reverse String Module Driver
 *
 * This kernel module implements a simple character device driver that allows
 * users to write data to the device, reverses the input string, and then 
 * allows users to read the reversed string back. It provides basic file 
 * operations such as open, read, write, and release.
 *
 * Key functionality:
 *
 * 1. Writing to the device takes input from the user, stores it in a buffer, 
 *    reverses the string, and then allows it to be read back in reversed form.
 *
 * 2. Reading from the device sends the reversed string back to the user in 
 *    chunks, ensuring all data is properly transferred.
 *
 * 3. The driver uses a predefined major number (241) and creates a device file
 *    under `/dev/rev_dev`. It also supports proper device registration
 *    and cleanup during module initialization and exit.
 *
 * Author: Ellen Ripley
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/mutex.h>

#define DEVICE_NAME "rev_dev"
#define CLASS_NAME  "rev_class"
#define BUFFER_SIZE 1024
#define MAJOR_NUM 241

static DEFINE_MUTEX(rev_mutex);
static char message[BUFFER_SIZE] = {0};
static short message_size;
static struct class*  reverseClass  = NULL;
static struct cdev rev_cdev;

static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char __user *, size_t, loff_t *);

static struct file_operations fops =
{
    .owner          = THIS_MODULE,
    .open           = dev_open,
    .read           = dev_read,
    .write          = dev_write,
    .release        = dev_release,
};

/*
 * reverse_string - Reverses the contents of a string.
 * This function takes a string and its length as inputs and reverses the string
 * in place. It is used to reverse the string stored in the device's message buffer.
 */
static void reverse_string(char *str, size_t len)
{
    size_t i;
    char temp;
    for (i = 0; i < len / 2; i++) {
        temp = str[i];
        str[i] = str[len - i -1];
        str[len - i -1] = temp;
    }
}

static char *rev_devnode(const struct device *dev, umode_t *mode)
{
    if (mode)
        *mode = 0666;
    return NULL;
}

/*
 * reverse_init - Initializes the reverse character device driver.
 * This function registers the character device with the system, assigns a
 * major number (241), creates the device class, and registers the device
 * itself. If any of these steps fail, it performs cleanup and exits with an error.
 */
static int __init reverse_init(void)
{
    int ret;
    dev_t dev_no = MKDEV(MAJOR_NUM, 0);
    struct device* reverseDevice = NULL;

    /* Register the device number */
    ret = register_chrdev_region(dev_no, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ALERT "%s: Failed to register a major number\n", DEVICE_NAME);
        return ret;
    }

    /* Initialize cdev structure and add it to kernel space */
    cdev_init(&rev_cdev, &fops);
    ret = cdev_add(&rev_cdev, dev_no, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_no, 1);
        printk(KERN_ALERT "%s: Failed to add cdev\n", DEVICE_NAME);
        return ret;
    }

    /* Register the device class */
    reverseClass = class_create(CLASS_NAME);
    if (IS_ERR(reverseClass)) {
        cdev_del(&rev_cdev);
        unregister_chrdev_region(dev_no, 1);
        printk(KERN_ALERT "%s: Failed to register device class\n", DEVICE_NAME);
        return PTR_ERR(reverseClass);
    }

    /* Set the devnode function to set default permissions */
    reverseClass->devnode = rev_devnode;

    /* Register the device driver */
    reverseDevice = device_create(reverseClass, NULL, dev_no, NULL, DEVICE_NAME);
    if (IS_ERR(reverseDevice)) {
        class_destroy(reverseClass);
        cdev_del(&rev_cdev);
        unregister_chrdev_region(dev_no, 1);
        printk(KERN_ALERT "%s: Failed to create the device\n", DEVICE_NAME);
        return PTR_ERR(reverseDevice);
    }

    printk(KERN_INFO "%s: Device class created correctly\n", DEVICE_NAME);
    return 0;
}

/*
 * reverse_exit - Cleans up and unloads the reverse character device driver.
 * This function unregisters the device, destroys the class, and removes
 * the character device from the system when the module is unloaded.
 */
static void __exit reverse_exit(void)
{
    dev_t dev_no = MKDEV(MAJOR_NUM, 0);
    device_destroy(reverseClass, dev_no);
    class_unregister(reverseClass);
    class_destroy(reverseClass);
    cdev_del(&rev_cdev);
    unregister_chrdev_region(dev_no, 1);
    printk(KERN_INFO "%s: Exit!\n", DEVICE_NAME);
}

/*
 * dev_open - Called when the device file is opened.
 * This function logs that the device has been opened, and it can also be
 * used to initialize any resources or reset state if necessary.
 */
static int dev_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "%s: Device has been opened\n", DEVICE_NAME);
    return 0;
}

/*
 * dev_release - Called when the device file is closed.
 * This function logs that the device has been successfully closed, and it 
 * can also be used to free any resources that were allocated during the 
 * device's use.
 */
static int dev_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "%s: Device successfully closed\n", DEVICE_NAME);
    return 0;
}

/*
 * dev_read - Handles reading data from the device.
 * This function copies the reversed string from the kernel buffer to the user
 * space buffer. It ensures that only valid data is read and updates the file
 * offset. It returns the number of bytes read or an error code.
 */
static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset)
{
    int ret;
    int bytes_to_read;
    
    mutex_lock(&rev_mutex);

    bytes_to_read = min(len, (size_t)(message_size - *offset));
    if (bytes_to_read <= 0) {
        printk(KERN_INFO "%s: End of message\n", DEVICE_NAME);
        mutex_unlock(&rev_mutex);
        return 0;
    }

    ret = copy_to_user(buffer, message + *offset, bytes_to_read);
    if (ret == 0) {
        *offset += bytes_to_read;
        printk(KERN_INFO "%s: Sent %d characters to the user\n", DEVICE_NAME, bytes_to_read);
        mutex_unlock(&rev_mutex);
        return bytes_to_read;
    } else {
        printk(KERN_ALERT "%s: Failed to send %d characters to the user\n", DEVICE_NAME, ret);
        mutex_unlock(&rev_mutex);
        return -EFAULT;
    }
}

/*
 * dev_write - Handles writing data to the device.
 * This function copies data from the user space buffer to the kernel buffer,
 * reverses the string, and stores the reversed result. It ensures the buffer
 * does not overflow and returns the number of bytes written or an error code.
 */
static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset)
{
    int ret;
    int bytes_to_write;

    mutex_lock(&rev_mutex);

    bytes_to_write = min(len, (size_t)(BUFFER_SIZE - 1));

    ret = copy_from_user(message, buffer, bytes_to_write);
    if (ret == 0) {
        message_size = bytes_to_write;
        message[message_size] = '\0';
        reverse_string(message, message_size);
        printk(KERN_INFO "%s: Received %d characters from the user\n", DEVICE_NAME, bytes_to_write);
        mutex_unlock(&rev_mutex);
        return bytes_to_write;
    } else {
        printk(KERN_ALERT "%s: Failed to receive %d characters from the user\n", DEVICE_NAME, ret);
        mutex_unlock(&rev_mutex);
        return -EFAULT;
    }
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ellen Ripley");
MODULE_DESCRIPTION("A char driver that reverses data");
MODULE_VERSION("1.0");
MODULE_ALIAS("char-major-241");

module_init(reverse_init);
module_exit(reverse_exit);
