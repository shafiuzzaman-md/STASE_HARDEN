#ifndef STUB_LIB_H
#define STUB_LIB_H

#include <stddef.h>
#include <stdint.h>

typedef uint64_t loff_t;

// stub memory ops
static inline void *dummy_kmalloc(size_t size) {
    static uint8_t buf[1024];
    return buf;
}
static inline void dummy_kfree(void *p) {}

static inline void *dummy_memset(void *dst, int val, size_t n) { return dst; }
static inline int dummy_sprintf(char *dst, const char *fmt) { return 0; }

static inline void dummy_copy_from_user(void *dst, const void *src, size_t n) {}
static inline void dummy_copy_to_user(void *dst, const void *src, size_t n) {}

static inline void dummy_printk(const char *fmt, ...) {}

#endif
