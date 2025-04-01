#ifndef __HEAP_H
#define __HEAP_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gfp.h>
#include <linux/slab.h>

#define INVALID_SIZE -(1 << 0)

void init_heap(void);
void free_heap(void);
void *alloc_chunk(unsigned int size);
void free_chunk(void *chunk);
int size_to_index(unsigned int size);
int index_to_size(unsigned int index);
void register_free_hook(void(*free_hook));
void register_alloc_hook(void(*alloc_hook));

#endif