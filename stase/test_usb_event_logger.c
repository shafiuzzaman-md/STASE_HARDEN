#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "klee/klee.h"
#include "instrumented_code/usb_event_logger.c"

/*
 * Stub for usb_register_notify, which would normally register
 * a notifier with the USB core in the kernel.
 */
int usb_register_notify(struct notifier_block *nb)
{
    printf("[stub] usb_register_notify called with nb=%p\n", (void*)nb);
    return 0;
}

/*
 * Stubbed kzalloc and kfree, mimicking kernel memory allocation.
 */
void *kzalloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void kfree(void *ptr)
{
    free(ptr);
}

/*
 * main - This is our userspace “driver” entry point.
 *        We call the logger init to trigger the bug.
 */
int main(void)
{
    printf("Running Dangling Pointer Demo (no Linux headers)...\n");
    usb_logger_init();  // This will intentionally cause a KLEE assertion
    return 0;
}
