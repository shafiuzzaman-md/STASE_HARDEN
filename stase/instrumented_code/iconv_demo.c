#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define CHUNK_SIZE 0x100
#define OVERFLOW_OFFSET 0x48

typedef struct chunk {
    union {
        struct chunk *next;         // Used when freed
        char data[CHUNK_SIZE];      // Used when allocated
    };
} chunk_t;

chunk_t *free_list = NULL;

// Allocate aligned memory
void *aligned_alloc(size_t alignment, size_t size) {
    void *ptr;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        perror("posix_memalign failed");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

// Allocate a chunk
chunk_t *allocate_chunk() {
    if (!free_list) {
        printf("No free chunks available!\n");
        return NULL;
    }
    chunk_t *allocated = free_list;
    free_list = free_list->next;
    
    printf("Allocated chunk at %p\n", allocated);
    return allocated;
}

// Free a chunk
void free_chunk(chunk_t *chunk) {
    chunk->next = free_list;
    free_list = chunk;
    printf("Freed chunk at %p\n", chunk);
}

// Initialize the heap
void initialize_heap(chunk_t **heap, size_t count) {
    *heap = aligned_alloc(CHUNK_SIZE, count * CHUNK_SIZE);
    for (size_t i = 0; i < count - 1; i++) {
        (*heap)[i].next = &(*heap)[i + 1];
    }
    (*heap)[count - 1].next = NULL;
    free_list = &(*heap)[0];
    printf("Heap initialized with %zu chunks.\n", count);
}

// Perform controlled overflow
void overflow(chunk_t *from, size_t offset, uint8_t value) {
    if (!from) return;
    klee_assert(offset < CHUNK_SIZE);
    uint8_t *target = (uint8_t *)from + CHUNK_SIZE + offset;
    *target = value;
    printf("Overflow wrote 0x%x to %p\n", value, target);
}

// Arbitrary write primitive
void perform_write(chunk_t *from, uint8_t value) {
    //if (!from) return;
    uint8_t *target = (uint8_t *)from;
    
      // ASSERTION FOR WRITE-WHAT-WHERE:
    // Check if the address being written to is symbolic (attacker-controlled)
    uintptr_t base = (uintptr_t)&safe_region[0];
    uintptr_t end  = (uintptr_t)&safe_region[255];

    //Assert that the write lands within the safe symbolic region
    klee_assert((uintptr_t)target >= base && (uintptr_t)target <= end);
    *target = value;
    printf("Write wrote 0x%x to %p\n", value, target);
}

// Show the free list layout
void display_free_list() {
    printf("Free list:\n");
    chunk_t *cur = free_list;
    while (cur) {
        uintptr_t next_ptr = (uintptr_t)cur->next;
        if (next_ptr == 0xdeadbeef || next_ptr <= 0x100) {
            printf("  %p (points to INVALID: 0x%lx)\n", cur, next_ptr);
            break;
        }
        printf("  %p -> %p\n", cur, cur->next);
        cur = cur->next;
    }
}

// === Attacker-controlled entrypoint ===
int driver_entry(uint8_t overflow_value, uint8_t write_value, size_t offset) {
    chunk_t *heap = NULL; // Pointer to the aligned heap
    long int myvariable[25] = {13};
    printf("myvariable init: 0x%lx @(%p)\n", myvariable[0], myvariable);
    initialize_heap(&heap, 3);
    display_free_list();

    // Allocate and free chunks
    chunk_t *a = allocate_chunk();
    chunk_t *b = allocate_chunk();
    chunk_t *c = allocate_chunk();
    free_chunk(a);
    free_chunk(b);
    free_chunk(c);
    display_free_list();

    // Allocate chunks again to set up arbitrary pointer in chunk A
    a = allocate_chunk();
    b = allocate_chunk();
    c = allocate_chunk();
    *(uintptr_t *)(a->data + OVERFLOW_OFFSET) = (uintptr_t)myvariable;
    printf("Arbitrary pointer set at %p: 0x%lx\n",
        a->data + OVERFLOW_OFFSET, *(uintptr_t *)(a->data + OVERFLOW_OFFSET));

    // Free the chunks again to reset the free list
    free_chunk(a);
    free_chunk(b);
    free_chunk(c);
    display_free_list();

    // Allocate chunk D and perform overflow from D into B
    chunk_t *d = allocate_chunk();
    overflow(d, offset, overflow_value);
    display_free_list();

    // Allocate from the corrupted free list
    chunk_t *e = allocate_chunk();
    chunk_t *f = allocate_chunk();
    chunk_t *g = allocate_chunk();  

    printf("Chunk E allocated at %p\n", e);
    printf("Chunk F allocated at %p\n", f);
    printf("Chunk G allocated at %p\n", g);

    perform_write(g, write_value);

    free(heap);  // Clean up allocated memory
    printf("myvariable: 0x%lx @(%p)\n", myvariable[0], myvariable);
    return 0;
}

// === Entry via main ===
// int main(int argc, char *argv[]) {
//     if (argc != 3) {
//         fprintf(stderr, "Usage: %s <overflow_byte> <write_byte>\n", argv[0]);
//         return 1;
//     }

//     uint8_t overflow_value = (uint8_t)strtoul(argv[1], NULL, 0);
//     uint8_t write_value = (uint8_t)strtoul(argv[2], NULL, 0);

//     driver_entry(overflow_value, write_value);
//     return 0;
// }
