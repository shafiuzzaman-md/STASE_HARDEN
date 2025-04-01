/*
 * thermal_char_device.c - Simulated Temperature Sensor Character Device
 *
 * This module simulates a temperature sensor by creating a character device
 * that users can read to obtain a periodically updated temperature value.
 *
 * The temperature is updated every 5 seconds by a kernel thread, and the
 * current temperature can be read from the device file at any time.
 *
 * Author: Richard Castle
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/random.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "thermal_char_device"
#define CLASS_NAME  "thermal_char_class"
#define MAJOR_NUM   243

static struct task_struct *temp_thread;
static int current_temperature = 0;
static struct class *temp_class  = NULL;
static DEFINE_MUTEX(temp_mutex);

/* Function prototypes */
static int     temp_open(struct inode *inode, struct file *filep);
static int     temp_release(struct inode *inode, struct file *filep);
static ssize_t temp_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset);

static struct file_operations temp_fops = {
	.owner   = THIS_MODULE,
	.open    = temp_open,
	.release = temp_release,
	.read    = temp_read,
};

/**
 * temp_sensor_thread - Kernel thread to update temperature.
 * @data: Pointer to thread data (unused).
 *
 * This function runs in a kernel thread and periodically updates
 * the current_temperature variable with a new random value.
 *
 * Return: Always returns 0.
 */
static int temp_sensor_thread(void *data)
{
	pr_debug("thermal_char_device: Temperature sensor thread started\n");

	while (!kthread_should_stop()) {
		int temp;

		/* Simulate temperature reading */
		temp = 25 + (get_random_u32() % 10); /* Random temp between 25 and 34 */

		mutex_lock(&temp_mutex);
		current_temperature = temp;
		mutex_unlock(&temp_mutex);

		ssleep(5);
	}

	pr_debug("thermal_char_device: Temperature sensor thread stopping\n");

	return 0;
}

/**
 * temp_open - Open function for the character device.
 * @inode: Pointer to inode structure.
 * @filep: Pointer to file structure.
 *
 * This function is called when the device file is opened.
 *
 * Return: Always returns 0.
 */
static int temp_open(struct inode *inode, struct file *filep)
{
	pr_debug("thermal_char_device: Device opened\n");
	return 0;
}

/**
 * temp_release - Release function for the character device.
 * @inode: Pointer to inode structure.
 * @filep: Pointer to file structure.
 *
 * This function is called when the device file is closed.
 *
 * Return: Always returns 0.
 */
static int temp_release(struct inode *inode, struct file *filep)
{
	pr_debug("thermal_char_device: Device closed\n");
	return 0;
}

/**
 * temp_read - Read function for the character device.
 * @filep: Pointer to file structure.
 * @buffer: User-space buffer to copy data to.
 * @len: Length of the user-space buffer.
 * @offset: Offset in the file (unused).
 *
 * This function copies the current temperature value to the user-space buffer.
 *
 * Return: Number of bytes read on success, negative error code on failure.
 */
static ssize_t temp_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset)
{
	char temp_str[16];
	int temp;
	ssize_t ret;

	/* Allow reading only once per open */
	if (*offset > 0)
		return 0; /* EOF */

	/* Retrieve the current temperature */
	mutex_lock(&temp_mutex);
	temp = current_temperature;
	mutex_unlock(&temp_mutex);

	/* Convert temperature to string */
	ret = snprintf(temp_str, sizeof(temp_str), "%d\n", temp);
	if (ret < 0)
		return ret;

	/* Ensure user buffer is large enough */
	if (len < ret)
		return -EINVAL;

	/* Copy data to user-space */
	if (copy_to_user(buffer, temp_str, ret))
		return -EFAULT;

	*offset += ret; /* Update file offset */

	return ret;
}

static char *thermal_devnode(const struct device *dev, umode_t *mode)
{
    if (mode)
        *mode = 0444;
    return NULL;
}

/**
 * temp_sensor_init - Module initialization function.
 *
 * This function sets up the character device, starts the kernel thread,
 * and creates the device file in /dev.
 *
 * Return: 0 on success, negative error code on failure.
 */
static int __init temp_sensor_init(void)
{
	int ret;
    static struct device* temp_device = NULL;
	pr_debug("thermal_char_device: Initializing module\n");

	/* Initialize current temperature */
	current_temperature = 25;

	/* Start the kernel thread */
	temp_thread = kthread_run(temp_sensor_thread, NULL, "temp_sensor_thread");
	if (IS_ERR(temp_thread)) {
		pr_err("thermal_char_device: Failed to create temperature sensor thread\n");
		return PTR_ERR(temp_thread);
	}

	/* Register character device with a fixed major number */
	ret = register_chrdev(MAJOR_NUM, DEVICE_NAME, &temp_fops);
	if (ret < 0) {
		pr_err("thermal_char_device: Failed to register character device\n");
		goto err_thread;
	}

	/* Create class */
	temp_class = class_create(CLASS_NAME);
	if (IS_ERR(temp_class)) {
		pr_err("thermal_char_device: Failed to create class\n");
		ret = PTR_ERR(temp_class);
		goto err_chrdev;
	}

    /* Set the devnode function to set default permissions */
    temp_class->devnode = thermal_devnode;

	/* Create device */
	temp_device = device_create(temp_class, NULL, MKDEV(MAJOR_NUM, 0), NULL, DEVICE_NAME);
	if (IS_ERR(temp_device)) {
		pr_err("thermal_char_device: Failed to create device\n");
		ret = PTR_ERR(temp_device);
		goto err_class;
	}

	mutex_init(&temp_mutex);

	pr_debug("thermal_char_device: Module initialized successfully\n");
	return 0;

err_class:
	class_destroy(temp_class);
err_chrdev:
	unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
err_thread:
	if (temp_thread)
		kthread_stop(temp_thread);
	return ret;
}

/**
 * temp_sensor_exit - Module cleanup function.
 *
 * This function stops the kernel thread, removes the character device,
 * and cleans up all allocated resources.
 */
static void __exit temp_sensor_exit(void)
{
	pr_debug("thermal_char_device: Exiting module\n");

	/* Stop the kernel thread */
	if (temp_thread)
		kthread_stop(temp_thread);

	/* Destroy device */
	device_destroy(temp_class, MKDEV(MAJOR_NUM, 0));

	/* Destroy class */
	class_destroy(temp_class);

	/* Unregister character device */
	unregister_chrdev(MAJOR_NUM, DEVICE_NAME);

	mutex_destroy(&temp_mutex);

	pr_debug("thermal_char_device: Module exited\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Richard Castle");
MODULE_DESCRIPTION("Simulated Temperature Sensor Character Device");
MODULE_VERSION("1.0");
MODULE_ALIAS("char-major-243");

module_init(temp_sensor_init);
module_exit(temp_sensor_exit);
