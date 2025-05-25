#ifndef STUB_LIB_H
#define STUB_LIB_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// Logging stubs
static inline void dummy_printk(const char *fmt, ...) {}

// Memory operation stubs
static inline void *dummy_kmalloc(size_t size) {
    static uint8_t dummy_heap[4096];
    return dummy_heap;
}

static inline void dummy_kfree(void *p) {}
static inline void dummy_memcpy(void *dst, const void *src, size_t n) {
    memcpy(dst, src, n);
}
static inline void *dummy_memset(void *s, int c, size_t n) {
    return memset(s, c, n);
}
static inline char *dummy_strchr(const char *s, int c) {
    return strchr(s, c);
}
static inline int dummy_sprintf(char *buf, const char *fmt, ...) {
    return 0;
}
static inline int dummy_copy_from_user(void *dst, const void *src, size_t n) {
    memcpy(dst, src, n);
    return 0;
}
static inline int dummy_copy_to_user(void *dst, const void *src, size_t n) {
    memcpy(dst, src, n);
    return 0;
}

#endif
