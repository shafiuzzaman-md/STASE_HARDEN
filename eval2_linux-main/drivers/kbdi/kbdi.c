#include <linux/module.h>
#include <linux/usb.h>
#include <linux/input.h>

/**
 * USB device ID table for the keyboard driver.
 *
 * This table defines the USB devices that this driver supports.
 */
static struct usb_device_id id_table[] = {
    { USB_DEVICE(0x03EB, 0x6124) },
    {}
};

MODULE_DEVICE_TABLE(usb, id_table);

/**
 * Global pointer to the input device structure.
 */
static struct input_dev *input_dev;

/**
 * usb_keyboard_probe - Probe function for the USB keyboard driver.
 *
 * @interface: The USB interface being probed.
 * @id: The USB device ID that matched this interface.
 *
 * This function is called when the USB interface is probed. It initializes
 * the input device and registers it with the input subsystem.
 *
 * Return: 0 on success, negative error code on failure.
 */
static int usb_keyboard_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    int ret;

    /**
     * Allocate a new input device.
     */
    input_dev = input_allocate_device();
    if (!input_dev) {
        printk(KERN_ERR "Failed to allocate input device\n");
        return -ENOMEM;
    }

    /**
     * Set up the input device properties.
     */
    input_dev->name = "Virtual Keyboard";
    input_dev->phys = "virtual/btn";
    input_dev->dev.parent = &interface->dev;

    /**
     * Set the event bits for the input device.
     */
    set_bit(EV_KEY, input_dev->evbit);
    set_bit(KEY_A, input_dev->keybit);

    /**
     * Register the input device with the input subsystem.
     */
    ret = input_register_device(input_dev);
    if (ret) {
        printk(KERN_ERR "Failed to register input device\n");
        input_free_device(input_dev);
        return ret;
    }

    /**
     * Simulate key presses and releases to test the device.
     */
    for (int i = 0; i < 20; ++i) {
        input_report_key(input_dev, KEY_A, 1);
        input_sync(input_dev);
        input_report_key(input_dev, KEY_A, 0);
        input_sync(input_dev);
    }

    return 0;
}

/**
 * usb_keyboard_disconnect - Disconnect function for the USB keyboard driver.
 *
 * @interface: The USB interface being disconnected.
 *
 * This function is called when the USB interface is disconnected. It unregisters
 * and frees the input device.
 */
static void usb_keyboard_disconnect(struct usb_interface *interface)
{
    if (input_dev) {
        input_unregister_device(input_dev);
        input_free_device(input_dev);
        input_dev = NULL;
    }
}

/**
 * usb_driver structure for the keyboard driver.
 *
 * This structure defines the USB driver and its callback functions.
 */
static struct usb_driver keyboard_driver = {
    .name       = "usb_keyboard",
    .id_table   = id_table,
    .probe      = usb_keyboard_probe,
    .disconnect = usb_keyboard_disconnect,
};

/**
 * usb_keyboard_init - Module initialization function.
 *
 * This function is called when the module is initialized. It registers the USB
 * driver with the USB subsystem.
 *
 * Return: 0 on success, negative error code on failure.
 */
static int __init usb_keyboard_init(void)
{
    return usb_register(&keyboard_driver);
}

/**
 * usb_keyboard_exit - Module exit function.
 *
 * This function is called when the module is unloaded. It deregisters the USB
 * driver from the USB subsystem.
 */
static void __exit usb_keyboard_exit(void)
{
    usb_deregister(&keyboard_driver);
}

module_init(usb_keyboard_init);
module_exit(usb_keyboard_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dave Green");
MODULE_DESCRIPTION("USB Keyboard Driver");
