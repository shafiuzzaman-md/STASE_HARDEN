#include <stdint.h>
#include "stub_lib.h"
#include "chain_globals.h"


int usb_logger_init(void) {
    // Initialize with a placeholder invalid value
    dangling_cb = (void *)0xdeadbeef;
    return 0;
}
