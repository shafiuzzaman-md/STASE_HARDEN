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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "dynamic_util.c"
// #include <linux/dynamic_util.h>
// #include <linux/init.h>
// #include <linux/mm.h>
// #include <asm/pgtable.h>
// #include <linux/string.h>
// #include <linux/notifier.h>
#define MESSAGE_SIZE 1024ull
#define NOTIFY_OK 0
char message_buffer[MESSAGE_SIZE];
extern void make_dynamic_area(void);
int UserDataIsInjectedOnStack = 0;
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
void load_message_to_memory(void) {
    char code_segment[MESSAGE_SIZE] = {0};

    if (!message_buffer[0])
        return;

    char* message_separator = strchr(message_buffer, '\0');

    if (!message_separator) {
        printf("Separator not found\n");
        return;
    }

    if (message_separator - message_buffer == MESSAGE_SIZE - 1) {
        printf("separator was at end\n");
        return;
    }

    char* code_start = message_separator + 1;
    char* code_end = message_buffer + MESSAGE_SIZE;
    size_t code_size = strlen(code_start);

    if (code_size > MESSAGE_SIZE) {
        printf("Code size exceeds buffer capacity\n");
        return;
    }
    UserDataIsInjectedOnStack = 1;//memcpy(code_segment, code_start, code_size);
    klee_assert(!UserDataIsInjectedOnStack);
    for (int i = 0; i < code_size-2; i++) {
        if (code_segment[i] == 0x00 &&
             code_segment[i+1] == 0xff && 
              code_segment[i+2] == 0x00)
            return; 
    }
    make_dynamic_area();



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
int kbmi_usb_notifier(unsigned long action) {
    load_message_to_memory();
    return NOTIFY_OK;
}

// USB notifier block structure
static struct {
    int (*notifier_call)(unsigned long action);
} my_nb = {
    .notifier_call = kbmi_usb_notifier,
};

// Module initialization function adapted for symbolic execution
static int kbmi_usb_init(void) {
    // Symbolically simulate registering the USB notifier
    klee_assume(my_nb.notifier_call != NULL);
    my_nb.notifier_call(0);  // Simulate a USB event
    return 0;
}


