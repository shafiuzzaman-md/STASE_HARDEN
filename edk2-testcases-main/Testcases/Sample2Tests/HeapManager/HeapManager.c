#include "HeapManager.h"

NOTIFY_PROTOCOL_READY_PROTOCOL      mHeapManagerReady;

HEAP_T Heap;
HEAP_METADATA_T HeapMd;
UINTN BlockSizes[MAX_PAGES] = {BLOCK_080, BLOCK_100, BLOCK_200};


EFI_STATUS
EFIAPI
GetBlockSize(
    IN UINTN Size,
    OUT INTN *BlockSize
)
{
    if (Size <= BLOCK_080)
        *BlockSize = B_080;
    if (Size <= BLOCK_100)
        *BlockSize = B_100;
    if (Size <= BLOCK_200)
        *BlockSize = B_200;
    else
        *BlockSize = -1;
    return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
AllocateChunk(
    IN UINTN ChunkSize,
    OUT CHUNK_T **AllocatedChunk
)
{
    if (ChunkSize < 0) {
        return EFI_INVALID_PARAMETER;
    }
    if (!Heap.FreeList[ChunkSize]) {
        DEBUG((DEBUG_ERROR, "No free chunks available!\n"));
        return EFI_OUT_OF_RESOURCES;
    }
    
    if (HeapMd.FreeHandleTop < 0) {
        HeapMd.FreeHandleTop = -1;
        DEBUG((DEBUG_ERROR, "No free handles available!\n"));
        return EFI_OUT_OF_RESOURCES;
    }

    (*AllocatedChunk) = Heap.FreeList[ChunkSize];
    Heap.FreeList[ChunkSize] = Heap.FreeList[ChunkSize]->Next;
    INTN Handle = HeapMd.FreeHandleTop--;
    HeapMd.HandleTable[Handle].ChunkPtr = *AllocatedChunk;
    HeapMd.HandleTable[Handle].ChunkSize = ChunkSize;
    HeapMd.HandleTable[Handle].InUse = 1;

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FreeChunk(
    IN INTN Handle
)
{
    if (Handle < 0 || Handle >= MAX_PAGES * MAX_ITEMS || !HeapMd.HandleTable[Handle].InUse) {
        DEBUG((DEBUG_ERROR, "Invalid selection!\n"));
        return EFI_INVALID_PARAMETER;
    }

    CHUNK_T *Chunk = HeapMd.HandleTable[Handle].ChunkPtr;
    UINTN ChunkSize = HeapMd.HandleTable[Handle].ChunkSize;

    Chunk->Next = Heap.FreeList[ChunkSize];
    Heap.FreeList[ChunkSize] = Chunk;

    HeapMd.HandleTable[Handle].InUse = 0;
    ++HeapMd.FreeHandleTop;
    DEBUG((DEBUG_ERROR, "Freed chunk(%d)(%d) at %p (0x%x)\n", Handle, HeapMd.FreeHandleTop, Chunk, BlockSizes[ChunkSize]));
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FreeAllocList()
{
    EFI_STATUS Status = EFI_SUCCESS;
    // frees in reverse order
    for (INTN i = (MAX_PAGES * MAX_ITEMS) - 1; i >= 0; i--) {
        if (HeapMd.HandleTable[i].InUse) {
            Status = FreeChunk(i);
            if (EFI_ERROR(Status))
            {
                return Status;
            }
        }
    }
    return Status;
}

EFI_STATUS
EFIAPI
DisplayFreeList(
    IN UINTN ChunkSize
)
{
    if (ChunkSize < 1 || ChunkSize > 3) {
        DEBUG((DEBUG_ERROR, "Invalid chunk size.\n"));
        return EFI_INVALID_PARAMETER;
    }

    CHUNK_T *Current = (CHUNK_T *)Heap.FreeList[ChunkSize-1];
    if (!Current) {
        DEBUG((DEBUG_ERROR, "Free list is empty\n"));
        return EFI_OUT_OF_RESOURCES;
    }
    DEBUG((DEBUG_ERROR, "Free list (0x%x):\n", BlockSizes[ChunkSize-1]));
    while (Current) {
        DEBUG((DEBUG_ERROR, "  Chunk at %p (points to %p)\n", Current, Current->Next));
        Current = Current->Next;
    }
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DisplayHeapMetadata()
{
    DEBUG((DEBUG_ERROR, "\nHandle Table:\n"));
    DEBUG((DEBUG_ERROR, "Handle | Address   | Size  | In Use\n"));
    DEBUG((DEBUG_ERROR, "-------|-----------|-------|-------\n"));
    for (INTN i = 0; i < MAX_PAGES * MAX_ITEMS; i++) {
        if (HeapMd.HandleTable[i].InUse) {
            DEBUG((DEBUG_ERROR, "%6d | %p | 0x%03x | Yes ", i, HeapMd.HandleTable[i].ChunkPtr, BlockSizes[HeapMd.HandleTable[i].ChunkSize]));
        } else {
            DEBUG((DEBUG_ERROR, "%6d | (Free)    | -     | No ", i));
        }
        if (HeapMd.FreeHandleTop == i)
            DEBUG((DEBUG_ERROR, "<--Top"));
        DEBUG((DEBUG_ERROR, "\n"));
    }
    DEBUG((DEBUG_ERROR, "\n"));
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
InitializePage(
    IN  UINTN  Size,
    OUT PAGE_T **Pages
)
{
    *Pages = (PAGE_T*)AllocateAlignedPages(EFI_SIZE_TO_PAGES(Size), 0x1000);
    if (*Pages == NULL) {
        DEBUG((DEBUG_ERROR, "AllocateAlignedPages failed"));
        return EFI_ABORTED;
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
InitializeHeapMetadata()
{
    DEBUG((DEBUG_ERROR, "InitializeHeapMetadata\n"));
    HeapMd.FreeHandleTop = MAX_PAGES * MAX_ITEMS - 1;
    for (UINTN i = 0; i < MAX_PAGES * MAX_ITEMS; i++) {
        HeapMd.HandleTable[i].ChunkPtr = NULL;
        HeapMd.HandleTable[i].ChunkSize = 0;
        HeapMd.HandleTable[i].InUse = 0;
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
InitializeHeap()
{
    EFI_STATUS Status = EFI_SUCCESS;
    DEBUG((DEBUG_ERROR, "InitializeHeap\n"));
    for (int i = 0; i < MAX_PAGES; i++) {
        Status = InitializePage(MAX_ITEMS * BlockSizes[i], &(Heap.Pages[i]));
        if(EFI_ERROR(Status))
        {
            return Status;
        }
        Heap.FreeList[i] = (CHUNK_T *)Heap.Pages[i];
        CHUNK_T *Current = (CHUNK_T *)Heap.FreeList[i];
        for (int j = 0; j < MAX_ITEMS-1; j++) {
            Current->Next = (CHUNK_T *)((UINT8 *)Current + BlockSizes[i]);
            Current = Current->Next;
        }
    }
    return Status;
}


EFI_STATUS
EFIAPI
HeapManagerEntryPoint(
    IN EFI_HANDLE       ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status;
    EFI_HANDLE HandleProtoc = NULL;
    EFI_HANDLE HandleNotify = NULL;
    HEAP_MANAGER_PROTOCOL *mHeapManager;
    UINTN      ProtocolSize = sizeof(HEAP_MANAGER_PROTOCOL);
    
    //
    //
    // Allocate a memory pool for the protocol
    Status = gSmst->SmmAllocatePool(
                    EfiRuntimeServicesData,
                    ProtocolSize,
                    (VOID **)&mHeapManager
                    );
    if (EFI_ERROR (Status)) {
        return Status;
    }

    //
    //
    // Initialize the protocol instance
    mHeapManager->GetBlockSize = GetBlockSize;
    mHeapManager->AllocateChunk = AllocateChunk;
    mHeapManager->FreeChunk = FreeChunk;
    mHeapManager->FreeAllocList = FreeAllocList;
    mHeapManager->DisplayFreeList = DisplayFreeList;
    mHeapManager->DisplayHeapMetadata = DisplayHeapMetadata;
    mHeapManager->InitializePage = InitializePage;
    mHeapManager->InitializeHeapMetadata = InitializeHeapMetadata;
    mHeapManager->InitializeHeap = InitializeHeap;

    //
    //
    // Install the protocol
    Status = gSmst->SmmInstallProtocolInterface(
                        &HandleProtoc,
                        &gEfiHeapManagerProtocolGuid,
                        EFI_NATIVE_INTERFACE,
                        mHeapManager
                        );
    if (EFI_ERROR (Status)) {
        return Status;
    }

    mHeapManagerReady.ReadyFlags = ~(0x00);
    Status = gSmst->SmmInstallProtocolInterface(
                    &HandleNotify,
                    &gEfiHeapManagerReadyProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mHeapManagerReady
                    );
    if (EFI_ERROR (Status)) {
        return Status;
    }

    return Status;
}