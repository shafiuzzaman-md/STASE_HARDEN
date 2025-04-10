#include <stdlib.h>
#include "klee/klee.h"

extern void trigger_allocator_reuse(char *freed, char *newbuf);

int main() {
    char *p1 = malloc(100); // allocate buffer A
    klee_make_symbolic(p1, 100, "p1");

    free(p1); // release A (this is done in the library)

    char *p2 = malloc(100); // allocator may reuse A's slot
    klee_make_symbolic(p2, 100, "p2");

    trigger_allocator_reuse(p1, p2); // observe overlap

    return 0;
}
