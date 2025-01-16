#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "klee/klee.h"
#include "instrumented_code/FaultTolerantWriteSmm.c"

/* Provide a stub CopyMem. In a real environment, CopyMem might be from 
   EDK II or standard libraries. Here we just use memcpy. */
void *CopyMem(void *Destination, const void *Source, size_t Length) {
  return memcpy(Destination, Source, Length);
}

BOOLEAN
FtwSmmIsBufferOutsideSmmValid (
  UINTN Buffer,
  UINTN BufferSize
)
{
    // Define start and end addresses of the buffer.
    UINTN BufferStart = Buffer;
    UINTN BufferEnd   = Buffer + BufferSize - 1;

  // SMRAM boundaries as symbolic variables
    UINTN SmramBaseAddress;
    UINTN SmramLimitAddress;

    // Make the SMRAM boundaries symbolic
    klee_make_symbolic(&SmramBaseAddress, sizeof(SmramBaseAddress), "SmramBaseAddress");
    klee_make_symbolic(&SmramLimitAddress, sizeof(SmramLimitAddress), "SmramLimitAddress");
    
    // Add constraints to ensure SmramBaseAddress < SmramLimitAddress
    klee_assume(SmramBaseAddress < SmramLimitAddress);


    // Check if the buffer is within SMRAM.
    if ((BufferStart >= SmramBaseAddress && BufferStart <= SmramLimitAddress) ||
        (BufferEnd >= SmramBaseAddress && BufferEnd <= SmramLimitAddress) ||
        (BufferStart < SmramBaseAddress && BufferEnd > SmramLimitAddress)) {
        // Buffer overlaps with SMRAM.
        return FALSE;
    }

    // Buffer is entirely outside SMRAM.
    return TRUE;
}

/* Mock definitions that match the instrumented code. */
//typedef int EFI_STATUS;
#define EFI_SUCCESS 0

int main(void) {
  printf("Running FaultTolerantWriteSmm Handler instrumentation demo...\n");

  /* We'll define a buffer for CommBuffer and a size. */
  char CommBuffer[128];
  unsigned long CommBufferSize = sizeof(CommBuffer);


  klee_make_symbolic(CommBuffer, sizeof(CommBuffer), "CommBuffer");
  klee_make_symbolic(&CommBufferSize, sizeof(CommBufferSize), "CommBufferSize");


  /* Call the instrumented handler */
  SmmFaultTolerantWriteHandler(
    NULL,   // DispatchHandle stub
    NULL,   // RegisterContext stub
    CommBuffer,
    &CommBufferSize
  );

  /* If we reach here, the assertion didn't fire. */
  printf("Handler returned successfully.\n");
  return 0;
}
