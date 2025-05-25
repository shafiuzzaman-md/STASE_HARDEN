#include <stdint.h>
#include "klee/klee.h"


#include "instrumented_code/iconv_demo.c"
int main() {
    uint8_t overflow_value;
    uint8_t write_value;
    size_t offset;
    klee_make_symbolic(&overflow_value, sizeof(overflow_value), "overflow_value");
    klee_make_symbolic(&write_value, sizeof(write_value), "write_value");
    klee_make_symbolic(&offset, sizeof(offset), "offset");
    klee_assume(offset > 0);
    driver_entry(overflow_value, write_value, offset);

    return 0;
}
