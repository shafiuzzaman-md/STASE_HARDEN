#include "chain_globals.h"
#include "stub_lib.h"

int proc_write(void *self, unsigned long action, void *dev) {
    dangling_cb = (void (*)(void))shell_buf;
    return 0;
}

int evt_notifier_init(void) {
    return proc_write(NULL, 0, NULL);
}

