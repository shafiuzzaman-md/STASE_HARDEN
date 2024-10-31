/** @file
   Executing some code on the Smm Stack Driver.

**/
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Uefi.h>
#include <Library/SmmServicesTableLib.h>
#include <Protocol/SmmMemoryAttribute.h>

#include "RdtscControl.h"

RDTSC_CONTROL_OPS   OpTable64 = {
  {0x0f, 0x20, 0xe0, 0x48, 0x0f, 0xba, 0xe8, 0x02, 0x0f, 0x22, 0xe0, 0xc3},
  {0x0f, 0x20, 0xe0, 0x48, 0x0f, 0xba, 0xf8, 0x02, 0x0f, 0x22, 0xe0, 0xc3},
  {0x0f, 0x20, 0xe0, 0x48, 0x83, 0xe0, 0x04, 0xc3} 
};

RDTSC_CONTROL_OPS   OpTable32 = {
  {0x0f, 0x20, 0xe0, 0x90, 0x0f, 0xba, 0xe8, 0x02, 0x0f, 0x22, 0xe0, 0xc3},
  {0x0f, 0x20, 0xe0, 0x90, 0x0f, 0xba, 0xf8, 0x02, 0x0f, 0x22, 0xe0, 0xc3},
  {0x0f, 0x20, 0xe0, 0x90, 0x83, 0xe0, 0x04, 0xc3}
};

/**
  This function is smi handler.

  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     Context         Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in,out] CommBuffer      A pointer to a collection of data in memory that will
                                 be conveyed from a non-SMM environment into an SMM environment.
  @param[in,out] CommBufferSize  The size of the CommBuffer.

  @retval EFI_SUCCESS            The interrupt was handled and quiesced. No other handlers
                                 should still be called.
**/
EFI_STATUS
EFIAPI
RdtscHandler (
  IN     EFI_HANDLE    DispatchHandle,
  IN     CONST VOID    *Context,
  IN OUT VOID          *CommBuffer,
  IN OUT UINTN         *CommBufferSize
  )
{
  EFI_STATUS            Status;
  RDTSC_CONTROL_OPS     OpTable;
  RDTSC_CONTROL_DATA    *CommData;
  UINTN                 DataSize;

  if (CommBuffer == NULL || CommBufferSize == NULL) {
    DEBUG((DEBUG_ERROR, "%a: missing communication buffer\n", __func__));
    return EFI_SUCCESS;
  }
  
  DataSize = *CommBufferSize;

  if (DataSize > sizeof(RDTSC_CONTROL_DATA)) {
    DEBUG((DEBUG_ERROR, "%a: invalid CommBufferSize -- too large !\n", __func__));
    return EFI_SUCCESS;
  }

  Status = gSmst->SmmAllocatePool (EfiRuntimeServicesData, DataSize, (VOID **)&CommData);

  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a: could not allocate smm pool !\n", __func__));
    return EFI_SUCCESS;
  }

  CopyMem((VOID *)CommData, (VOID *)CommBuffer, DataSize);

  if (AsmReadCs() & 0x18) {
    CopyMem((VOID *)&OpTable, (VOID *)&OpTable64, sizeof(RDTSC_CONTROL_OPS));
  } else {
    CopyMem((VOID *)&OpTable, (VOID *)&OpTable32, sizeof(RDTSC_CONTROL_OPS));
  }

  if (CommData->Type == TYPE_SET_RDTSC) {
    ((void (*)())&OpTable.SetRdtsc)();
  } else if (CommData->Type == TYPE_CLR_RDTSC) {
    ((void (*)())&OpTable.ClrRdtsc)();
  } else if (CommData->Type != TYPE_GET_RDTSC) {
    DEBUG((DEBUG_ERROR, "%a: unfamiliar rdtsc control type !\n", __func__));
    CommData->Status = 0x0f;
  }

  *CommBufferSize = DataSize;
  CommData->Result = ((char (*)())&OpTable.GetRdtsc)();
  CopyMem((VOID *)CommBuffer, (VOID *)CommData, DataSize);

  return Status;
}

/**
  The module Entry Point of the SMM driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
RdtscControlInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                          Status;
  EDKII_SMM_LEGACY_PROTOCOL           *SmmLegacy;
  EDKII_SMM_LEGACY_REGISTER_CONTEXT   SwContext;
  EFI_HANDLE                          SwHandle;

  Status                = gSmst->SmmLocateProtocol (&gEfiSmmLegacyProtocolGuid, NULL, (VOID **)&SmmLegacy);
  
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a: failed to local SmmLegacy protocol\n", __func__));
    return Status;
  }
    
  SwContext.InputValue  = 0x4b;
  Status                = SmmLegacy->Register (SmmLegacy, RdtscHandler, &SwContext, &SwHandle);

  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a: failed to register with Smm Legacy\n", __func__));
    return Status;
  }

  return Status;
}
