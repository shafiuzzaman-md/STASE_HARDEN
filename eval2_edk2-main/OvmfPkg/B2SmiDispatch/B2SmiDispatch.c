/** @file
  SMM SwDispatch2 for B2.


**/

#include <PiSmm.h>
#include <PiMm.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Uefi.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SmmMemLib.h>
#include <Library/LockBoxLib.h>
#include <Library/MemoryAllocationLib.h>
#include <IndustryStandard/Q35MchIch9.h>
#include <Protocol/MmCpuIo.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h> 
#include <Library/MmServicesTableLib.h> 
#include <Library/PcdLib.h>
#include <Uefi/UefiBaseType.h>
#include <Library/RngLib.h>
#include <Protocol/Rng.h>
#include <Protocol/SmmBase2.h>
#include <Protocol/SmmCpu.h>
#include <Library/IoLib.h>

#define SMM_CONTROL_PORT          0xB2
#define SMM_DATA_PORT             0xB3
#define B2SWSMI                   0xB4

typedef struct {
  UINTN         Signature;
  LIST_ENTRY    Link;
  EFI_HANDLE    DispatchHandle;
  UINTN         SwSmiInputValue;
  UINTN         DispatchFunction;
} EFI_SMM_SW_DISPATCH2_CONTEXT;

EFI_SMM_SYSTEM_TABLE2 *mSmst = NULL;
EFI_SMM_CPU_PROTOCOL  *mSmmCpuProtocol;
LIST_ENTRY            mSmmSwDispatch2Queue = INITIALIZE_LIST_HEAD_VARIABLE (mSmmSwDispatch2Queue);


/**
  Find SmmSwDispatch2Context by SwSmiInputValue.

  @param[in] SwSmiInputValue      The value to indentify the SmmSwDispatch2 context

  @return Pointer to EFI_SMM_SW_DISPATCH2_CONTEXT context
**/
EFI_SMM_SW_DISPATCH2_CONTEXT *
FindContextBySwSmiInputValue (
  IN UINTN  SwSmiInputValue
  )
{
  LIST_ENTRY                    *Node;
  EFI_SMM_SW_DISPATCH2_CONTEXT  *Dispatch2Context;

  Node = mSmmSwDispatch2Queue.ForwardLink;
  for ( ; Node != &mSmmSwDispatch2Queue; Node = Node->ForwardLink) {
    Dispatch2Context = BASE_CR (Node, EFI_SMM_SW_DISPATCH2_CONTEXT, Link);
    if (Dispatch2Context->SwSmiInputValue == SwSmiInputValue) {
      return Dispatch2Context;
    }
  }

  return NULL;
}

/**
  This function is for SMM Communication call back

  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     RegisterContext         Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in,out] CommBuffer      A pointer to a collection of data in memory that will
                                 be conveyed from a non-SMM environment into an SMM environment.
  @param[in,out] CommBufferSize  The size of the CommBuffer.

  @retval EFI_SUCCESS            The interrupt was handled and quiesced. No other handlers
                                 should still be called.
**/
STATIC
EFI_STATUS
EFIAPI
B2SmiDispatchHandle (
  IN EFI_HANDLE DispatchHandle,
  IN CONST VOID *RegisterContext OPTIONAL,
  IN OUT VOID   *CommBuffer     OPTIONAL,
  IN OUT UINTN  *CommBufferSize OPTIONAL
  )
{
  EFI_STATUS                    Status;
  EFI_SMM_SW_CONTEXT            SwContext;
  UINTN                         Index;
  EFI_SMM_SW_DISPATCH2_CONTEXT  *Context;
  EFI_SMM_HANDLER_ENTRY_POINT2  DispatchFunction;
  EFI_SMM_SW_REGISTER_CONTEXT   DispatchContext;
  UINTN                         Size;
  EFI_SMM_SAVE_STATE_IO_INFO    IoInfo;

  //
  //
  // Construct new context
  SwContext.SwSmiCpuIndex = 0;
  SwContext.CommandPort   = IoRead8 (SMM_CONTROL_PORT);
  SwContext.DataPort      = IoRead8 (SMM_DATA_PORT);

  //
  //
  // Try to find which CPU trigger SWSMI
  for (Index = 0; Index < gSmst->NumberOfCpus; Index++) {
    Status = mSmmCpuProtocol->ReadSaveState (
                                mSmmCpuProtocol,
                                sizeof (IoInfo),
                                EFI_SMM_SAVE_STATE_REGISTER_IO,
                                Index,
                                &IoInfo
                                );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (IoInfo.IoPort == SMM_CONTROL_PORT) {
      SwContext.SwSmiCpuIndex = Index;
      DEBUG ((DEBUG_VERBOSE, "CPU index = 0x%x/0x%x\n", Index, gSmst->NumberOfCpus));
      break;
    }
  }

  if (SwContext.CommandPort == 0) {
    DEBUG ((DEBUG_VERBOSE, "NOT SW SMI.\n"));
    Status = EFI_SUCCESS;
    goto End;
  }

  //
  //
  // Search context
  Context = FindContextBySwSmiInputValue (SwContext.CommandPort);
  if (Context == NULL) {
    DEBUG ((DEBUG_INFO, "No handler for SMI value 0x%x.\n", SwContext.CommandPort));
    Status = EFI_SUCCESS;
    goto End;
  }

  //
  //
  // Dispatch
  DispatchContext.SwSmiInputValue = SwContext.CommandPort;
  Size                            = sizeof (SwContext);
  DispatchFunction                = (EFI_SMM_HANDLER_ENTRY_POINT2)Context->DispatchFunction;
  Status                          = DispatchFunction (DispatchHandle, &DispatchContext, &SwContext, &Size);

End:
  return EFI_SUCCESS;
}

/**
  The module Entry Point of the B2SmiDispatch SMM driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS        The entry point is executed successfully.
  @retval Other              Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
B2SmiDispatchEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS                     Status;
  EFI_SMM_BASE2_PROTOCOL         *mSmmBase;
  EFI_SMM_SW_DISPATCH2_PROTOCOL  *mSwDispatch;
  EFI_SMM_SW_REGISTER_CONTEXT    mSwContext;
  EFI_HANDLE                     mSwHandle;
  DEBUG ((DEBUG_ERROR, "%a: Start\n", __FUNCTION__));
  //
  // Locate SmmBase2 protocol
  //
  Status = gBS->LocateProtocol(
                  &gEfiSmmBase2ProtocolGuid,
                  NULL,
                  (VOID **)&mSmmBase
                  );
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a: Couldn't find SmmBase2 protocol: %r\n", __FUNCTION__, Status));
    return Status;
  }

  //
  // Locate sm system table
  //
  Status = mSmmBase->GetSmstLocation(
                       mSmmBase,
                       &mSmst
                       );
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a: Couldn't find mSmst: %r\n", __FUNCTION__, Status));
    return Status;
  }

  //
  // Locate SwDispatch protocol
  //
  Status = mSmst->SmmLocateProtocol(
                    &gEfiSmmSwDispatch2ProtocolGuid,
                    NULL,
                    (VOID **)&mSwDispatch
                    );
  if (EFI_ERROR(Status)) {
    DEBUG(( DEBUG_ERROR, "%a: Couldn't find SmmSwDispatch protocol: %r\n", __FUNCTION__, Status));
    return Status;
  }

  //
  // Locate PI SMM CPU protocol
  //
  Status = gSmst->SmmLocateProtocol(
                    &gEfiSmmCpuProtocolGuid,
                    NULL,
                    (VOID **)&mSmmCpuProtocol
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for callbacks
  //
  mSwContext.SwSmiInputValue = B2SWSMI; 
  Status = mSwDispatch->Register(
                          mSwDispatch, 
                          B2SmiDispatchHandle, 
                          &mSwContext,
                          &mSwHandle
                          );
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "%a: registered swsmi handler\n", __FUNCTION__));
    return Status;
  }
 
  //
  // Install NULL to DXE data base as notify
  //
  ImageHandle = NULL;
  Status      = gBS->InstallProtocolInterface(
                       &ImageHandle,
                       &gEfiB2SmiDispatchProtocolGuid,
                       EFI_NATIVE_INTERFACE,
                       NULL
                       );
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "%a: installed protocol.\n", __FUNCTION__));
    return Status;
  }

  return EFI_SUCCESS;
}
