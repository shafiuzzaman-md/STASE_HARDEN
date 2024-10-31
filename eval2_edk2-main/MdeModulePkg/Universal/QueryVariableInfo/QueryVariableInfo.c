/** @file
  Query Variable Driver.

  Caution: This module requires additional review when modified.
  This driver will have external input - communicate buffer in SMM mode.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.
**/

#include "QueryVariableInfo.h"
BOOLEAN inDxe = TRUE;

/**
  Communication service SMI Handler entry.
  This SMI handler provides service for QueryVariableInfo.

  Caution: This function may receive untrusted input.
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
QueryVariableInfoHandler (
  IN     EFI_HANDLE    DispatchHandle,
  IN     CONST VOID    *Context,
  IN OUT VOID          *CommBuffer,
  IN OUT UINTN         *CommBufferSize
  )
{
  QUERY_VARIABLE_INFO *CommData;
  UINTN               Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;

  if (CommBuffer == NULL || CommBufferSize == NULL) {
    return EFI_SUCCESS;
  }
  CommData = (QUERY_VARIABLE_INFO *)CommBuffer;

  if (inDxe) {
    gRT->QueryVariableInfo(
                  Attributes,
                  &CommData->MaximumVariableStorageSize,
                  &CommData->RemainingVariableStorageSize,
                  &CommData->MaximumVariableSize
                  );
  } 

  return EFI_SUCCESS;
}

//
// Entry point function of this driver.
//
EFI_STATUS
EFIAPI
QueryVariableInfoEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS Status;
  EFI_HANDLE DispatchHandle;

  //
  // Register QueryVariableInfo handler function
  //
  Status = gSmst->SmiHandlerRegister(
                    QueryVariableInfoHandler, 
                    &gEfiQueryVariableInfoCommGuid, 
                    &DispatchHandle
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Install NULL to DXE data base as notify
  //
  ImageHandle = NULL;
  Status      = gBS->InstallProtocolInterface (
                       &ImageHandle, 
                       &gEfiQueryVariableInfoProtocolGuid, 
                       EFI_NATIVE_INTERFACE, 
                       NULL
                       );
  ASSERT_EFI_ERROR (Status);
  
  inDxe = FALSE;
  return Status;
}
