#include "heap.h"

#define INITIAL_ENTRIES 1
#define MIN_SIZE 4 // 2 ^ 4
#define MAX_SIZE 13 // 2 ^ 13
#define SIZES (PAGE_SHIFT - MIN_SIZE + 1)
#define PAGE_OF(x) ((struct page_t *)((long long)x & ~(0xfff)))
#define DEBUG 0
#define CACHE_PAGES 8

static struct kmem_cache *heap_cache = NULL;

struct chunk_t {
	struct chunk_t *next;
};

struct page_t {
	struct heap_entry_t *parent;
	struct page_t *next;
};

struct heap_entry_t {
	struct heap_entry_t *next;
	struct chunk_t *freelist;
	struct page_t *this;
};

struct {
	struct heap_entry_t *entry_list[SIZES];
	struct page_t *unused_pages;
	void (*alloc_hook)(unsigned int size);
	void (*free_hook)(void *ptr);
} heap;

void chunk_page(struct heap_entry_t *entry, struct page_t *ptr,
		unsigned int size);
void free_unused_pages(void);
void free_entry(int index);
struct heap_entry_t *alloc_entry(int index);
void cache_pages(void);

int size_to_index(unsigned int size)
{
	if (size > PAGE_SIZE)
		panic("[CONV] size_to_index: size too large");

	if (size == 0)
		return 0;

	int index = (int)sizeof(size) * 8 - __builtin_clz(size);
	if (1 << (index - 1) == size)
		index--;

	return index - MIN_SIZE < 0 ? 0 : index - MIN_SIZE;
}

int index_to_size(unsigned int index)
{
	return 1 << (index + MIN_SIZE);
}

void chunk_page(struct heap_entry_t *entry, struct page_t *ptr,
		unsigned int size)
{
	for (char *addr = (char *)ptr + PAGE_SIZE - size;; addr -= size) {
		struct chunk_t *chunk_ptr = (struct chunk_t *)addr;
		chunk_ptr->next = entry->freelist;
		entry->freelist = chunk_ptr;

		if (addr == (char *)ptr)
			break;
	}
}

void cache_pages(void)
{
	for (int i = 0; i < CACHE_PAGES; i++) {
		struct page_t *page_cache =
			kmem_cache_zalloc(heap_cache, GFP_KERNEL);
		page_cache->next = heap.unused_pages;
		heap.unused_pages = page_cache;
	}
}

void init_heap(void)
{
	heap_cache = kmem_cache_create("conv_heap", PAGE_SIZE, PAGE_SIZE,
				       SLAB_HWCACHE_ALIGN, NULL);
	heap.alloc_hook = NULL;
	heap.free_hook = NULL;

	if (DEBUG)
		printk("[CONV] heap free_hook address: 0x%llx\n",
		       (long long)&heap.free_hook);

	for (int i = 0; i < SIZES; i++) {
		for (int j = 0; j < INITIAL_ENTRIES; j++) {
			struct heap_entry_t *prev = heap.entry_list[i];
			struct heap_entry_t *curr = alloc_entry(i);

			curr->next = prev;
			heap.entry_list[i] = curr;
		}
	}

	cache_pages();
}

void free_unused_pages(void)
{
	for (struct page_t *curr_page = heap.unused_pages; curr_page != NULL;) {
		struct page_t *saved = curr_page->next;
		kfree(curr_page);
		curr_page = saved;
	}
}

void free_entry(int index)
{
	for (struct heap_entry_t *curr_entry = heap.entry_list[index];
	     curr_entry != NULL;) {
		kfree(curr_entry->this);
		struct heap_entry_t *saved = curr_entry->next;
		kfree(curr_entry);
		curr_entry = saved;
	}
}

void free_heap(void)
{
	free_unused_pages();

	for (int i = 0; i < SIZES; i++) {
		free_entry(i);
	}

	kmem_cache_destroy(heap_cache);
}

struct heap_entry_t *alloc_entry(int index)
{
	struct page_t *page_use;

	if (heap.unused_pages) {
		page_use = heap.unused_pages;
		heap.unused_pages = heap.unused_pages->next;
	} else {
		page_use = kmem_cache_zalloc(heap_cache, GFP_KERNEL);
		cache_pages();
	}

	struct heap_entry_t *curr =
		kzalloc(sizeof(struct heap_entry_t), GFP_KERNEL);

	curr->this = page_use;
	chunk_page(curr, page_use, index_to_size(index));
	return curr;
}

void register_free_hook(void(*free_hook))
{
	heap.free_hook = free_hook;
}

void register_alloc_hook(void(*alloc_hook))
{
	heap.alloc_hook = alloc_hook;
}

void *alloc_chunk(unsigned int size)
{
	if (heap.alloc_hook)
		heap.alloc_hook(size);

	void *ret = NULL;
	int index = size_to_index(size);

	for (struct heap_entry_t *curr_entry = heap.entry_list[index];
	     curr_entry != NULL; curr_entry = curr_entry->next) {
		if (curr_entry->freelist != NULL) {
			ret = curr_entry->freelist;
			curr_entry->freelist = curr_entry->freelist->next;
			break;
		}
	}

	if (ret == NULL) {
		struct heap_entry_t *new = alloc_entry(index);

		if (DEBUG)
			printk("[CONV] new: 0x%llx\n", (long long)new);

		new->next = heap.entry_list[index];
		heap.entry_list[index] = new;
		ret = new->freelist;
		new->freelist = new->freelist->next;
	}

	if (DEBUG)
		printk("[CONV] allocated 0x%llx from entry of size %d\n",
		       (long long)ret, index_to_size(index));

	return ret;
}

void free_chunk(void *chunk)
{
	if (heap.free_hook)
		heap.free_hook(chunk);

	struct page_t *page = PAGE_OF(chunk);

	for (int i = 0; i < SIZES; i++) {
		for (struct heap_entry_t *curr_entry = heap.entry_list[i];
		     curr_entry != NULL; curr_entry = curr_entry->next) {
			if (page == curr_entry->this) {
				((struct chunk_t *)chunk)->next =
					curr_entry->freelist;
				curr_entry->freelist = chunk;

				if (DEBUG)
					printk("[CONV] freed a chunk (0x%llx) in the entry of size %d which was on page 0x%llx\n",
					       (long long)chunk,
					       index_to_size(i),
					       (long long)page);

				return;
			}
		}
	}

	panic("[CONV] free_chunk: corrupted heap");
}
