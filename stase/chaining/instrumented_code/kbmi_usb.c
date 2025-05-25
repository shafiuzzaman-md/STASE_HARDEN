#include "chain_globals.h"
#include "stub_lib.h"

char message_buffer[1024] = {0};

// definitions
uint8_t shell_buf[1024];
uint8_t *shell_capture = shell_buf;
void (*dangling_cb)(void) = 0;
uint8_t shell_byte;  // define symbolic byte

void load_message_to_memory(void) {
    shell_buf[0] = shell_byte;  // write symbolic byte into shell buffer
}

int kbmi_usb_init(void) {
    load_message_to_memory();
    return 0;
}
