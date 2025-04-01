#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "klee/klee.h"

int StackIsExecutable = 1;

#include "instrumented_code/conv_46_OOB_WRITE.c"

#define SYMBOLIC_INPUT_SIZE 4
#define CONV_UTF32_UTF8 1024
#define CONV_LIST_SIZE 128

// Stub: kernel memory operations
int copy_from_user(void *to, const void *from, size_t n) {
    memcpy(to, from, n);
    return 0;
}
int copy_to_user(void *to, const void *from, size_t n) {
    return 0;
}

// Stub: heap management
void *alloc_chunk(size_t size) {
    void *buf = malloc(size);
    klee_make_symbolic(buf, size, "alloc_chunk");
    return buf;
}
void free_chunk(void *ptr) {
    // no-op
}

int main() {
    // ===== Entrypoint args =====
    struct file *file = NULL;
    unsigned int cmd = CONV_UTF32_UTF8;

    // === Symbolic argument that will control conversion ===
    static struct user_data_t ud;
    klee_make_symbolic(&ud, sizeof(ud), "arg");

    // Setup a valid index in conv_list
    static uint32_t symbolic_input[4];
    klee_make_symbolic(symbolic_input, sizeof(symbolic_input), "symbolic_input");

    int fixed_index = 3;
    conv_list[fixed_index].string = symbolic_input;
    conv_list[fixed_index].size = sizeof(symbolic_input);
    conv_list[fixed_index].f = UTF32;

    // Force ud.index = fixed_index to ensure alignment with conv_list
    ud.index = fixed_index;

    // Make output buffer concrete (will be written to by conversion)
    static char outbuf[64];
    ud.out = outbuf;

    // Cast to kernel arg
    unsigned long arg = (unsigned long)&ud;

    // === Call entrypoint ===
    conv_ioctl(file, cmd, arg);

    return 0;
}