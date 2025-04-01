#ifndef __CONV_H
#define __CONV_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#define ERR_B64_DECODE -(1 << 0)

void escape_utf8(uint8_t *utf8, size_t last, size_t j);
size_t utf32_char_to_utf8(uint32_t codepoint, uint8_t *utf8);
size_t utf32_string_to_utf8(const uint32_t *utf32, uint8_t *utf8,
			    size_t utf32_len);
size_t utf8_encoding_length(uint32_t codepoint);
size_t count_utf8_bytes(const uint32_t *utf32, size_t utf32_len);
static long conv_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

#endif