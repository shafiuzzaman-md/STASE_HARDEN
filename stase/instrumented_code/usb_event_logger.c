/*
 * USB Device Event Logger Kernel Module
 *
 * This Linux kernel module registers for notifications of USB device events
 * (such as when USB devices are added or removed). It logs detailed information 
 * about the devices, including their vendor ID, product ID, manufacturer, 
 * product name, serial number, device path, speed, class, subclass, and protocol. 
 * It also logs information about each interface in the active configuration of 
 * the USB device.
 *
 * Author: Stanley Hudson
 */

// #include <linux/module.h>
#include <stdio.h> // #include <linux/kernel.h>
#include <string.h> // #include <linux/init.h>
//extern int  usb_register_notify(struct notifier_block *nb); /* Stubs for "kernel-like" functions. Actual definitions are in the driver. */
extern void *kzalloc(size_t size);
extern void kfree(void *ptr);

_Bool DANGLING_POINTER_USED = 0;// MODULE_LICENSE("GPL");
// MODULE_AUTHOR("Stanley Hudson");
// MODULE_DESCRIPTION("USB Device Event Logger");

/*
 * usb_notify - Callback function for USB device events.
 * @self: Notifier block (unused).
 * @action: The action performed on the USB device (e.g., USB_DEVICE_ADD or USB_DEVICE_REMOVE).
 * @dev: Pointer to the USB device structure (struct usb_device).
 *
 * This function is called whenever a USB device is added or removed. It logs
 * detailed information about the device, such as vendor ID, product ID, 
 * manufacturer, product name, serial number, speed, and class. It also logs 
 * details about each interface of the device.
 *
 * Return: NOTIFY_OK to indicate that the event was successfully handled.
 */
static int usb_notify(struct notifier_block *self, unsigned long action, void *dev)
{
    struct usb_device *udev = dev;
    struct usb_host_interface *iface_desc;
    int i;

    // /* Log the action (add or remove) */
    // switch (action) {
    // case USB_DEVICE_ADD:
    //     printk(KERN_INFO "USB device added:\n");
    //     break;
    // case USB_DEVICE_REMOVE:
    //     printk(KERN_INFO "USB device removed:\n");
    //     break;
    // default:
    //     printk(KERN_INFO "USB event: action=%lu\n", action);
    //     break;
    // }

    // /* Log detailed information about the device if available */
    // if (udev) {
    //     printk(KERN_INFO "Bus: %03d, Device: %03d\n", udev->bus->busnum, udev->devnum);
    //     printk(KERN_INFO "Vendor ID: %04x, Product ID: %04x\n",
    //            le16_to_cpu(udev->descriptor.idVendor), le16_to_cpu(udev->descriptor.idProduct));
    //     printk(KERN_INFO "Manufacturer: %s, Product: %s, SerialNumber: %s\n",
    //            udev->manufacturer ? udev->manufacturer : "Unknown",
    //            udev->product ? udev->product : "Unknown",
    //            udev->serial ? udev->serial : "Unknown");
    //     printk(KERN_INFO "Device Path: %s\n", udev->devpath);
    //     printk(KERN_INFO "Device Class: %u, Subclass: %u, Protocol: %u\n",
    //            udev->descriptor.bDeviceClass,
    //            udev->descriptor.bDeviceSubClass,
    //            udev->descriptor.bDeviceProtocol);

    //     /* Log details about each interface in the active configuration */
    //     if (udev->actconfig) {
    //         for (i = 0; i < udev->actconfig->desc.bNumInterfaces; i++) {
    //             struct usb_interface *interface = udev->actconfig->interface[i];
    //             if (interface && interface->cur_altsetting) {
    //                 iface_desc = interface->cur_altsetting;
    //                 printk(KERN_INFO "Interface Number: %d\n", iface_desc->desc.bInterfaceNumber);
    //                 printk(KERN_INFO "Interface Class: %u, SubClass: %u, Protocol: %u\n",
    //                        iface_desc->desc.bInterfaceClass,
    //                        iface_desc->desc.bInterfaceSubClass,
    //                        iface_desc->desc.bInterfaceProtocol);
    //             }
    //         }
    //     }
    // }

    return 0; 
}

/* This pointer will become dangling after we free it. */
static struct notifier_block *usb_nb_dangling = NULL;



/*
 * usb_logger_init - Module initialization function.
 *
 * This function is called when the module is loaded. It registers the USB
 * notifier to receive notifications of USB device events.
 *
 * Return: Always returns 0 (success).
 */
int usb_logger_init(void)
{   DANGLING_POINTER_USED = 1; 
    //klee_assert(!DANGLING_POINTER_USED);
    return 0;
}

/*
 * usb_logger_exit - Stub cleanup.
 * In a real module, you might free resources here, but we've already
 * freed the pointer incorrectly. We'll just log a message.
 */
void usb_logger_exit(void)
{
    printf("Stub: usb_logger_exit called.\n");
}
