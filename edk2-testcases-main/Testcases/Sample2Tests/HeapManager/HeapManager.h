#ifndef __HEAP_MANAGER_H__
#define __HEAP_MANAGER_H__

#include <Library/MmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/SmmMemLib.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/SmmMemoryAttribute.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/IoLib.h>

extern EFI_GUID gEfiHeapManagerProtocolGuid;
extern EFI_GUID gEfiHeapManagerReadyProtocolGuid;

#define MAX_ITEMS 10
#define MAX_PAGES 3
#define BLOCK_080 0x80
#define BLOCK_100 0x100
#define BLOCK_200 0x200
#define MAX_BYTES BLOCK_200
#define MAX_FILENAME 32
#define NUL_TERMINATOR_LENGTH 4

enum {
    B_080 = 0,
    B_100,
    B_200
};

typedef struct CHUNK {
   struct CHUNK *Next;
} CHUNK_T;

typedef struct PAGE {
    CHUNK_T *Self;
} PAGE_T;

typedef struct {
    CHUNK_T *ChunkPtr;
    UINTN ChunkSize;
    INTN InUse;
} Handle_Entry_T;

typedef struct {
    Handle_Entry_T HandleTable[MAX_PAGES * MAX_ITEMS];
    INTN FreeHandleTop;
} HEAP_METADATA_T;


typedef struct HEAP {
    PAGE_T *Pages[MAX_PAGES];
    CHUNK_T *FreeList[MAX_ITEMS];
} HEAP_T;

extern HEAP_T Heap;
extern HEAP_METADATA_T HeapMd;
extern UINTN BlockSizes[MAX_PAGES];

typedef
EFI_STATUS
(EFIAPI *HEAP_MANAGER_GET_BLOCK_SIZE)(
    IN UINTN Size,
    OUT INTN *BlockSize
);

typedef
EFI_STATUS
(EFIAPI *HEAP_MANAGER_ALLOCATE_CHUNK)(
    IN UINTN ChunkSize,
    OUT CHUNK_T **AllocatedChunk
);

typedef
EFI_STATUS
(EFIAPI *HEAP_MANAGER_FREE_CHUNK)(
    IN INTN Handle
);

typedef
EFI_STATUS
(EFIAPI *HEAP_MANAGER_FREE_ALLOC_LIST)();

typedef
EFI_STATUS
(EFIAPI *HEAP_MANAGER_DISPLAY_FREE_LIST)();

typedef
EFI_STATUS
(EFIAPI *HEAP_MANAGER_DISPLAY_HEAP_METADATA)();

typedef
EFI_STATUS
(EFIAPI *HEAP_MANAGER_INITIALIZE_PAGE)(
    IN  UINTN  Size,
    OUT PAGE_T **Pages
);

typedef
EFI_STATUS
(EFIAPI *HEAP_MANAGER_INITIALIZE_HEAP_METADATA)();

typedef
EFI_STATUS
(EFIAPI *HEAP_MANAGER_INITIALIZE_HEAP)();

typedef struct _HEAP_MANAGER_PROTOCOL {
    HEAP_MANAGER_GET_BLOCK_SIZE            GetBlockSize;
    HEAP_MANAGER_ALLOCATE_CHUNK            AllocateChunk;
    HEAP_MANAGER_FREE_CHUNK                FreeChunk;
    HEAP_MANAGER_FREE_ALLOC_LIST           FreeAllocList;
    HEAP_MANAGER_DISPLAY_FREE_LIST         DisplayFreeList;
    HEAP_MANAGER_DISPLAY_HEAP_METADATA     DisplayHeapMetadata;
    HEAP_MANAGER_INITIALIZE_PAGE           InitializePage;
    HEAP_MANAGER_INITIALIZE_HEAP_METADATA  InitializeHeapMetadata;
    HEAP_MANAGER_INITIALIZE_HEAP           InitializeHeap;
} HEAP_MANAGER_PROTOCOL;

typedef struct _NOTIFY_PROTOCOL_READY_PROTOCOL {
  UINT32      ReadyFlags;
} NOTIFY_PROTOCOL_READY_PROTOCOL;

#endif // __HEAP_MANAGER_H__