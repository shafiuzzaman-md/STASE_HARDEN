#include "chain_globals.h"
#include "stub_lib.h"

int kbdi_init(void) {
    if (!dangling_cb)
        return 0;

    // shell_byte already made symbolic in PRE_CHECK
   // klee_assert((uint64_t)dangling_cb == (uint64_t)shell_buf);
   // klee_assert(shell_buf[0] != shell_byte);  // compare to shared symbolic

    return 0;
}
