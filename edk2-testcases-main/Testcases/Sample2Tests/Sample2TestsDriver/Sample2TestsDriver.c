#include "Sample2TestsDriver.h"

HEAP_MANAGER_PROTOCOL *mHeapManager = NULL;
CHAR_CONVERTER_PROTOCOL *mCharConverter = NULL;


CHAR16* w_msg = L"Hello\0";
void (*funcptr)();

VOID winning_msg() {
    DEBUG((DEBUG_ERROR, "pwned\n"));
}

VOID welcome_msg() {
    DEBUG((DEBUG_ERROR, "%s", w_msg));
    DEBUG((DEBUG_ERROR, " - this Sample has a WMP.\n"));
    DEBUG((DEBUG_ERROR, "Here's a couple addresses - just cuz: \n\tWinning Msg function (%p)\n\tA function pointer (%p)\n", winning_msg, &funcptr));
}


EFI_STATUS convert(SAMPLE2_TESTS_DRIVER_COMM_BUFFER  **Buffer)
{
  EFI_STATUS Status = EFI_SUCCESS;
  CHAR_CONVERTER_DATA *Input = (CHAR_CONVERTER_DATA*)(*Buffer)->Data;
  switch((*Buffer)->Conversion)
  {
    case STANDARD:
      Status = mCharConverter->StandardConvert((Input->Data), &Input->Size, mHeapManager);
      break;
    case LIBXML2:
      Status = mCharConverter->Libxml2Convert((Input->Data), &Input->Size, mHeapManager);
      break;
    case PKEXEC:
      Status = mCharConverter->PkexecConvert((Input->Data), &Input->Size, mHeapManager);
      break;
    default:
      DEBUG((DEBUG_ERROR, "INVALID Input!!\n"));
      Status = EFI_INVALID_PARAMETER;
      break;
  }
  return Status;
}

/**
  This SMI handler provides access to hardware management services exposed by the Thermal Sensor,
  Fan Controller, Voltage Regulator, LED Controller and Process Firmware protocols.

  @param[in] DispatchHandle    The unique handle assigned to this handler by SmiHandlerRegister() 
  @param[in] Context           Points to an optional handler context which was specified when the
                               handler was registered.
  @param[in] CommBuffer        A pointer to a collection of data in memory that will be conveyed
                               from a non-SMM environment into an SMM environment.
  @param[in] CommBufferSize    The size of the CommBuffer

  @retval    EFI_SUCCESS       The interrupt was handled and quiesced.
  @retval    Others            Other errors as indicated.
**/
EFI_STATUS
EFIAPI
Sample2TestsDriverHandler (
  IN      EFI_HANDLE    DispatchHandle,
  IN      CONST VOID    *Context OPTIONAL,
  IN OUT  VOID          *CommBuffer OPTIONAL,
  IN OUT  UINTN         *CommBufferSize OPTIONAL
  )
{

    SAMPLE2_TESTS_DRIVER_COMM_BUFFER  *TmpCommBuffer;

    if (CommBuffer == NULL || CommBufferSize == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    TmpCommBuffer = (SAMPLE2_TESTS_DRIVER_COMM_BUFFER *)CommBuffer;

    switch(TmpCommBuffer->Operation)
    {
        case CONVERT:
            convert(&TmpCommBuffer);
            break;
        case FREE_LIST:
            mHeapManager->FreeAllocList();
            break;
        case DISPLAY_LIST:
            UINTN ChunkSize = TmpCommBuffer->Data[0];
            if (ChunkSize == 0) {
                ChunkSize = 1; // Default to 1 if no size is specified
            }
            mHeapManager->DisplayFreeList(ChunkSize);
            break;
        case DISPLAY_METADATA:
            mHeapManager->DisplayHeapMetadata();
            break;
        case MSG:
          funcptr();
          break;
        default:
            break;
    }

  return EFI_SUCCESS;
}


/**
  This protocol notify callback locates the Heap Manager protocol.

  @param[in] Protocol               The GUID of the notifying protocol.
  @param[in] Interface              The interface to which the protocol notification was bound.
  @param[in] Handle                 Points to an optional handler context which was specified when the
                                    handler was registered.
  
  @retval    EFI_SUCCESS            The protocol was located.
  @retval    Others                 Other errors as indicated.
**/
EFI_STATUS
EFIAPI
HeapManagerReadyCallback (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS Status;

  Status = gSmst->SmmLocateProtocol(&gEfiHeapManagerProtocolGuid, NULL, (VOID **)&mHeapManager);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "%a: Failed to locate ProcessFirmwareProtocol. Status: %r\n", __func__, Status));
    return Status;
  }
  DEBUG((DEBUG_ERROR, "HeapManagerReadyCallback\n"));
  mHeapManager->InitializeHeap();
  mHeapManager->InitializeHeapMetadata();
  
  return EFI_SUCCESS;
}

/**
  This protocol notify callback locates the Heap Manager protocol.

  @param[in] Protocol               The GUID of the notifying protocol.
  @param[in] Interface              The interface to which the protocol notification was bound.
  @param[in] Handle                 Points to an optional handler context which was specified when the
                                    handler was registered.
  
  @retval    EFI_SUCCESS            The protocol was located.
  @retval    Others                 Other errors as indicated.
**/
EFI_STATUS
EFIAPI
CharConverterReadyCallback (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS Status;

  Status = gSmst->SmmLocateProtocol(&gEfiCharConverterProtocolGuid, NULL, (VOID **)&mCharConverter);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "%a: Failed to locate ProcessFirmwareProtocol. Status: %r\n", __func__, Status));
    return Status;
  }
  DEBUG((DEBUG_ERROR, "CharConverterReadyCallback\n"));
  
  return EFI_SUCCESS;
}

/**
  Shared entry point of the module

  @retval EFI_SUCCESS           The initialization finished successfully.
  @retval EFI_OUT_OF_RESOURCES  Allocate memory error.
  @retval EFI_INVALID_PARAMETER Workspace or Spare block does not exist.
**/
EFI_STATUS
EFIAPI
Sample2TestsDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  EFI_HANDLE    DispatchHandle;
  VOID          *HeapManagerNotifyRegistration;
  VOID          *CharConverterNotifyRegistration;
  
  Status = gSmst->SmmLocateProtocol(&gEfiHeapManagerProtocolGuid, NULL, (VOID **)&mHeapManager);
  if (EFI_ERROR(Status)) {
    //
    //
    // Load the protocols when ProcessFirmware is ready
    Status = gSmst->SmmRegisterProtocolNotify(
      &gEfiHeapManagerReadyProtocolGuid,
      HeapManagerReadyCallback,
      &HeapManagerNotifyRegistration
      );
    if (EFI_ERROR (Status)) {
      DEBUG((DEBUG_ERROR, "%a: Could not register protocol notify\n", __func__));
      return Status;
    }
  }
  else
  {
    mHeapManager->InitializeHeap();
    mHeapManager->InitializeHeapMetadata();
  }

  funcptr = welcome_msg;
  funcptr(); 

  //
  //
  // Load the protocols when ProcessFirmware is ready
  Status = gSmst->SmmRegisterProtocolNotify(
                    &gEfiCharConverterReadyProtocolGuid,
                    CharConverterReadyCallback,
                    &CharConverterNotifyRegistration
                    );
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a: Could not register protocol notify\n", __func__));
    return Status;
  }


  //
  //
  // Register the SMI handler
  Status = gSmst->SmiHandlerRegister(
                    Sample2TestsDriverHandler,
                    &gEfiSample2TestsDriverCommGuid,
                    &DispatchHandle
                    );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "%a: Failed to register Sample2TestsDriverHandler. Status: %r\n", __func__, Status));
    return Status;
  }

  return Status;
}