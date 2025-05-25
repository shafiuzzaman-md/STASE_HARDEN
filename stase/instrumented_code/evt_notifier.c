#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "stub_lib.h"

#define BUFFER_SIZE 256
#define MAJOR_NUM_DR_W 242
#define DEVICE_NAME "dr_w_device"

static int offset = 0x372;
static char message[BUFFER_SIZE] = {0};
static short message_size;

typedef struct {
    int (*notifier_call)(void *, unsigned long, void *);
} notifier_block;

static int proc_write(notifier_block *self, unsigned long action, void *dev)
{
    char *buf = NULL;

    if (offset < 0 || offset > 0x600) {
        dummy_printk("Invalid offset\n");
        return 0;
    }

    // Model: execute symbolic function pointer based on offset
    void (*fp)(void) = (void (*)(void))offset;
    klee_assume((uintptr_t)fp == offset);  // symbolic link
    fp();  // symbolic call (models indirect control flow)

    // Vulnerable buffer logic preserved
    if (action == 1) {
        dummy_printk("Device added\n");
        buf = dummy_kmalloc(1024);
        if (buf) {
            dummy_memset(buf, 0, 1024);
            dummy_sprintf(buf, "Initializing device...");
            for (int i = 0; i < 1024; i++) {
                buf[i] = (buf[i] + i) % 256;
            }
            dummy_kfree(buf);
        }
    }

    unsigned long sum = 0;
    for (int i = 0; i < 100; i++) {
        sum += i * offset;
    }
    dummy_printk("Computation done: %lu\n", sum);
    return 0;
}

static notifier_block event_handler = {
    .notifier_call = proc_write,
};

// File operation stubs
static ssize_t dr_w_dev_read(void *filep, char *buffer, size_t len, loff_t *offset)
{
    int bytes_to_read = BUFFER_SIZE - *offset;
    if (bytes_to_read <= 0) {
        dummy_printk("End of message\n");
        return 0;
    }
    dummy_copy_to_user(buffer, message + *offset, bytes_to_read);
    *offset += bytes_to_read;
    return bytes_to_read;
}

static ssize_t dr_w_dev_write(void *filep, const char *buffer, size_t len, loff_t *f_pos)
{
    char input[BUFFER_SIZE] = {0};
    long value;

    if (len > BUFFER_SIZE - 1)
        return -1;

    dummy_copy_from_user(input, buffer, len);
    input[len] = '\0';

    // symbolic parse logic (simulate input)
    klee_make_symbolic(&value, sizeof(value), "user_offset");
    klee_assume(value >= 0x0 && value <= 0x600);
    offset = (int)value;

    return len;
}

static int evt_notifier_init(void)
{
    event_handler.notifier_call = proc_write;

    // skip char device setup; simulate callback only
    dummy_printk("Initializing event notifier\n");
    proc_write(&event_handler, 1, NULL); // simulate device add
    return 0;
}

static void evt_notifier_exit(void)
{
    dummy_printk("Dr. W device unregistered\n");
}
