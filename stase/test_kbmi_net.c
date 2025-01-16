#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "klee/klee.h"

int StackIsExecutable = 1;

#include "instrumented_code/kbmi_net.c"

int main(void) {
    klee_make_symbolic(message_buffer, sizeof(message_buffer), "message_buffer");
    /* Ensure null termination for safety */
    message_buffer[MESSAGE_SIZE - 1] = '\0';

    // Initialize the module, setting up symbolic execution from here
    return kbmi_net_init();
}
