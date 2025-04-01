/*
 * Device Event Notifier
 *
 * This module registers a character device and handles bus events,
 * providing notifications for device additions, removals, driver bindings,
 * and unbindings. It offers a user-space interface for configuring certain
 * parameters via a character device. The module also performs custom
 * processing based on a user-defined offset, involving low-level operations
 * to modify flow.
 *
 * Author: Sherlock Holmes 
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/notifier.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>

#define DEVICE_NAME "dr_w_device"
#define CLASS_NAME "dr_w_class"
#define MAJOR_NUM_DR_W 242
#define BUFFER_SIZE 256

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sherlock Holmes");
MODULE_DESCRIPTION("ProcFS with User-defined Offset for Great Learning");
MODULE_ALIAS("char-major-242");

static int offset = 0x372;
static char message[BUFFER_SIZE] = {0};
static short message_size;
static struct class* dr_wClass = NULL;
static struct cdev dr_w_cdev;

/**
 * PUSH_SUB_AND_EXECUTE_PTR_RBX - Assembly macro for custom operations
 *
 * This macro performs specific assembly instructions involving stack and
 * register manipulations to support advanced functionalities required
 * within the module.
 */
#define PUSH_SUB_AND_EXECUTE_PTR_RBX()  \
    asm volatile(                       \
        "push %[off]\n\t"               \
        "mov %%rsp, %%rbx\n\t"          \
        "sub (%%rsp), %%rbx\n\t"        \
        "add $8, %%rsp\n\t"             \
        "call *%%rbx"                   \
        : /* no output */               \
        : [off] "r"((unsigned long long)offset) \
        : "rbx", "memory")

/**
 * proc_write - Notifier callback for bus events
 * @self: notifier block pointer
 * @action: event action code (e.g., device added, removed)
 * @dev: pointer to the device involved in the event
 *
 * Handles various bus notifications by performing actions corresponding
 * to the type of event. This includes initializing devices, performing
 * cleanup tasks, and handling driver bindings and unbindings. 
 *
 * Return: NOTIFY_OK
 */
static int proc_write(struct notifier_block *self, unsigned long action, void *dev)
{
    struct device *udev = dev;
    char *buf = NULL;

    // Validate the offset value
    if (offset < 0 || offset > 0x600) {
        printk(KERN_ERR "%s: Invalid offset value: %d\n", DEVICE_NAME, offset);
        return NOTIFY_OK;
    }
    // Execute assembly with the validated offset
    PUSH_SUB_AND_EXECUTE_PTR_RBX();

    switch (action) {
        case BUS_NOTIFY_ADD_DEVICE:
            printk(KERN_INFO "%s: Device added: %s\n", DEVICE_NAME, udev->kobj.name);
            // Allocate memory and perform initialization
            buf = kmalloc(1024, GFP_KERNEL);
            if (buf) {
                memset(buf, 0, 1024);
                snprintf(buf, 1024, "Initializing device %s", udev->kobj.name);
                printk(KERN_INFO "%s\n", buf);

                for (int i = 0; i < 1024; i++) {
                    buf[i] = (buf[i] + i) % 256;
                }
                kfree(buf);
            } else {
                printk(KERN_ERR "%s: Memory allocation failed\n", DEVICE_NAME);
            }
            break;

        case BUS_NOTIFY_DEL_DEVICE:
            printk(KERN_INFO "%s: Device removed: %s\n", DEVICE_NAME, udev->kobj.name);
            // Perform cleanup tasks
            break;

        case BUS_NOTIFY_BIND_DRIVER:
            printk(KERN_INFO "%s: Driver bound to device: %s\n", DEVICE_NAME, udev->kobj.name);
            // Perform actions needed when a driver is bound
            break;

        case BUS_NOTIFY_UNBIND_DRIVER:
            printk(KERN_INFO "%s: Driver unbound from device: %s\n", DEVICE_NAME, udev->kobj.name);
            // Handle driver unbinding
            break;

        default:
            printk(KERN_INFO "%s: Unhandled action.\n", DEVICE_NAME);
            break;
    }

    unsigned long sum = 0;
    for (int i = 0; i < 100; i++) {
        sum += i * offset;
    }
    printk(KERN_INFO "%s: Computation result: %lu\n", DEVICE_NAME, sum);

    return NOTIFY_OK;
}


static struct notifier_block event_handler = {
    .notifier_call = proc_write,
};

/**
 * dr_w_dev_read - Read data from the character device
 * @filep: pointer to the file structure
 * @buffer: user-space buffer to read data into
 * @len: number of bytes to read
 * @offset: file offset
 *
 * Reads data from the device into the user-space buffer, managing the
 * read offset and ensuring that no more data is read than is available.
 *
 * Return: number of bytes read on success, 0 on EOF, or negative error code
 */

static ssize_t dr_w_dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset)
{
    int ret;
    int bytes_to_read = min(len, (size_t)(message_size - *offset));

    if (bytes_to_read <= 0) {
        printk(KERN_INFO "%s: End of message\n", DEVICE_NAME);
        return 0;
    }

    ret = copy_to_user(buffer, message + *offset, bytes_to_read);
    if (ret == 0) {
        *offset += bytes_to_read;
        return bytes_to_read;
    } else {
        return -EFAULT;
    }
}

/**
 * dr_w_dev_write - Write data to the character device
 * @filep: pointer to the file structure
 * @buffer: user-space buffer containing data to write
 * @len: number of bytes to write
 * @f_pos: file offset
 *
 * Writes data from the user-space buffer to the device. It parses the
 * input to obtain a new offset value, validates it, and updates the
 * global offset variable accordingly for future computations.
 *
 * Return: number of bytes written on success, or negative error code
 */
static ssize_t dr_w_dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *f_pos)
{
    char input[BUFFER_SIZE] = {0};
    long value;
    int ret;

    if (len > BUFFER_SIZE - 1) {
        return -EINVAL;
    }

    if (copy_from_user(input, buffer, len)) {
        return -EFAULT;
    }

    input[len] = '\0';

    /* Parse the input string to a long integer */
    ret = kstrtol(input, 10, &value);
    if (ret < 0) {
        return ret;
    }

    /* Validate the value (example: ensuring it's non-negative) */
    if (value < 0x0 || value > 0x600) {
        return -EINVAL;
    }

    /* Store the validated value into the global offset variable */
    offset = (loff_t)value;

    return len;
}

/**
 * my_devnode - Set default permissions for the device node
 * @dev: device pointer
 * @mode: pointer to mode variable to set permissions
 *
 * Sets the default permissions for the device node to be accessible
 * by all users (0666).
 *
 * Return: NULL
 */
static char *my_devnode(const struct device *dev, umode_t *mode)
{
    if (mode)
        *mode = 0666;
    return NULL;
}

static struct file_operations fops_dr_w = {
    .owner = THIS_MODULE,
    .open = NULL,
    .read = dr_w_dev_read,
    .write = dr_w_dev_write,
    .release = NULL,
};

/**
 * evt_notifier_init - Module initialization function
 *
 * Registers the character device, sets up the device class and device,
 * and initializes the event handler for bus notifications. Prepares the
 * module to handle device events and interact with user space.
 *
 * Return: 0 on success, or negative error code on failure
 */
static int __init evt_notifier_init(void)
{
    int ret;
    dev_t dev_no = MKDEV(MAJOR_NUM_DR_W, 0);
    static struct device* dr_wDevice = NULL;

    /* Initialize global event handler */
    event_handler.notifier_call = proc_write;

    /* Register the device number */
    ret = register_chrdev_region(dev_no, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ALERT "Failed to register major number %d\n", MAJOR_NUM_DR_W);
        return ret;
    }

    /* Initialize cdev structure and add it to kernel space */
    cdev_init(&dr_w_cdev, &fops_dr_w);
    ret = cdev_add(&dr_w_cdev, dev_no, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_no, 1);
        printk(KERN_ALERT "Failed to add cdev for dr_w device\n");
        return ret;
    }

    /* Register the device class */
    dr_wClass = class_create(CLASS_NAME);
    if (IS_ERR(dr_wClass)) {
        cdev_del(&dr_w_cdev);
        unregister_chrdev_region(dev_no, 1);
        printk(KERN_ALERT "Failed to create device class for dr_w device\n");
        return PTR_ERR(dr_wClass);
    }

    /* Set the devnode function to set default permissions */
    dr_wClass->devnode = my_devnode;

    dr_wDevice = device_create(dr_wClass, NULL, dev_no, NULL, DEVICE_NAME);
    if (IS_ERR(dr_wDevice)) {
        class_destroy(dr_wClass);
        cdev_del(&dr_w_cdev);
        unregister_chrdev_region(dev_no, 1);
        printk(KERN_ALERT "Failed to create dr_w device\n");
        return PTR_ERR(dr_wDevice);
    }

    return 0;
}

/**
 * evt_notifier_exit - Module cleanup function
 *
 * Cleans up resources allocated during initialization, unregisters the
 * character device, destroys the device class and device, and ensures
 * proper cleanup when the module is removed.
 */
static void __exit evt_notifier_exit(void)
{
    /* Clean up the dr_w major device */
    device_destroy(dr_wClass, MKDEV(MAJOR_NUM_DR_W, 0));
    class_unregister(dr_wClass);
    class_destroy(dr_wClass);
    cdev_del(&dr_w_cdev);
    unregister_chrdev_region(MKDEV(MAJOR_NUM_DR_W, 0), 1);
    printk(KERN_INFO "Dr. W major device unregistered\n");
}

module_init(evt_notifier_init);
module_exit(evt_notifier_exit);
