#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void trigger_allocator_reuse(char *freed, char *newbuf) {
    free(freed);  // freed buffer

    // Print addresses to observe reuse
    printf("ADDR freed: %p\n", freed);
    printf("ADDR newbuf: %p\n", newbuf);

    // Potential reuse and overlap if newbuf == freed
    strcpy(newbuf, "OVERWRITE");  // If reuse occurred → UAF
}
