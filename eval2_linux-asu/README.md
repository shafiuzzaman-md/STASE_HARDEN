# eval2_linux

from https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.6.25.tar.gz

### Conv Driver  
The new driver, located in the `drivers/conv` directory, implements custom logic for converting between different character sets. Currently, it supports translation from UTF-32 to UTF-8, but the design is intended to allow users to allocate multiple strings (currently limited to 128) in various formats and apply an unlimited number of transformations. For more details, refer to the `drivers/conv/conv.c` file.

### Driver Heap  
The driver also includes a custom heap implementation, found in the `drivers/conv/heap.c` file. This heap manages memory in chunks that follow powers of two, with a maximum size corresponding to a single page. It keeps track of chunks by size and automatically adds new entries when memory becomes unavailable. When a chunk is freed, it is simply placed back into the appropriate freelist. The heap provides customizable `alloc` and `free` hooks for users who wish to modify the default memory allocation behavior. Additionally, to optimize performance, the heap also maintains a freelist pointer for pages, pre-allocating a set number of pages for future use.
