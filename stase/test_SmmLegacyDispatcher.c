#include "ECH.c"


#include "instrumented_code/SmmLegacy.c"

// Test handler function
EFI_STATUS EFIAPI TestHandlerFunction(
    IN EFI_HANDLE DispatchHandle,
    IN CONST VOID *RegisterContext,
    IN OUT VOID *CommBuffer,
    IN OUT UINTN *CommBufferSize
) {
    return EFI_SUCCESS;
}

// Function to register test handlers
void RegisterTestHandlers() {
    EDKII_SMM_LEGACY_REGISTER_CONTEXT RegContext;
    EFI_HANDLE DispatchHandle;
    EFI_STATUS Status;

    // Handler with InputValue = 0x10
    RegContext.InputValue = 0x10;
    Status = SmmLegacyRegister(&gSmmLegacy, TestHandlerFunction, &RegContext, &DispatchHandle);
    if (EFI_ERROR(Status)) {
        printf("Failed to register handler with InputValue 0x10\n");
        exit(1);
    }

    // Handler with InputValue = 0x20
    RegContext.InputValue = 0x20;
    Status = SmmLegacyRegister(&gSmmLegacy, TestHandlerFunction, &RegContext, &DispatchHandle);
    if (EFI_ERROR(Status)) {
        printf("Failed to register handler with InputValue 0x20\n");
        exit(1);
    }
}

void test_SmmLegacyDispatcher() {
    // Manually initialize the dispatch queue
    mSmmLegacyDispatchQueue.ForwardLink = &mSmmLegacyDispatchQueue;
    mSmmLegacyDispatchQueue.BackLink = &mSmmLegacyDispatchQueue;

    // Register handlers
    RegisterTestHandlers();

    // Set up symbolic variables
    EFI_HANDLE handle;
    klee_make_symbolic(&handle, sizeof(handle), "DispatchHandle");

    void* RegisterContext = NULL; // Assuming no context is used for simplicity

    // Define an array of test buffer sizes
    const size_t test_sizes[] = {64, 128, 256, 512, 1024};
    const int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

    for (int i = 0; i < num_sizes; ++i) {
        UINTN CommBufferSize = test_sizes[i];

        // Allocate a buffer with the concrete size CommBufferSize
        void* CommBuffer = malloc(CommBufferSize);
        if (!CommBuffer) {
            printf("Memory allocation for CommBuffer failed\n");
            exit(1);
        }

        // Make the buffer symbolic
        klee_make_symbolic(CommBuffer, CommBufferSize, "CommBuffer");

        // Running the dispatcher test
        EFI_STATUS status = SmmLegacyDispatcher(handle, RegisterContext, CommBuffer, &CommBufferSize);
        free(CommBuffer);
    }
}

void InitializeMockSmst()
{
    // Zero out the structure
  memset(&MockSmst, 0, sizeof(MockSmst));
  // Assign the stubbed SmmAllocatePool function
  MockSmst.SmmAllocatePool = StubSmmAllocatePool;

  // Point gSmst to our mock SMST
  gSmst = &MockSmst;
}

int main() {
    stase_init_env();
     InitializeMockSmst();
    test_SmmLegacyDispatcher();
    return 0;
}