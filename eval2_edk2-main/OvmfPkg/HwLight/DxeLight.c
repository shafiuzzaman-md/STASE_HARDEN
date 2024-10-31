/** @file
  This file implements the Dxe Light Control.
  Disable Light SMM at ExitBootServices.

**/

#include "SmmLight.h"

EFI_SMM_COMMUNICATION_PROTOCOL    *mSmmCommunication = NULL;
VOID                              *mCommBuffer       = NULL;

/**
  Initialize the communicate buffer using DataSize and Function number.

  @param[out]      CommunicateBuffer The communicate buffer. Caller should free it after use.
  @param[out]      DataPtr           Points to the data in the communicate buffer. Caller should not free it.
  @param[in]       Function          The function number used to initialize the communicate header.

**/
STATIC
EFI_STATUS
InitializeCommBufferLight (
  OUT  VOID     **CommunicationBuffer,
  OUT  VOID     **DataPtr,
  IN   UINTN    Function
  )
{
  EFI_SMM_COMMUNICATE_HEADER        *SmmCommunicationHeader;
  SMM_LIGHT_FUNCTION_COMMUNICATION  *SmmLightFunctionCommunication;
  UINTN                             SmmCommunicationSize;

  //
  // Init the communicate buffer. The buffer data size is:
  // SMM_COMMUNICATE_HEADER_SIZE + SMM_LIGHT_FUNCTION_COMMUNICATION_SIZE.
  //
  SmmCommunicationHeader     = mCommBuffer;
  SmmCommunicationSize = SMM_COMMUNICATE_HEADER_SIZE + SMM_LIGHT_FUNCTION_COMMUNICATION_SIZE;
  if (SmmCommunicationSize > EFI_PAGES_TO_SIZE (1)) {
    return EFI_ACCESS_DENIED;
  }
  ZeroMem (SmmCommunicationHeader, SmmCommunicationSize);

  //
  // Setup Communication Buffer Data.
  //
  CopyGuid (&SmmCommunicationHeader->HeaderGuid, &gEfiLightStatusCodeCommunicationGuid);
  SmmCommunicationHeader->MessageLength = SMM_LIGHT_FUNCTION_COMMUNICATION_SIZE;

  SmmLightFunctionCommunication = (SMM_LIGHT_FUNCTION_COMMUNICATION *) SmmCommunicationHeader->Data;

  SmmLightFunctionCommunication->Function = Function;
  SmmLightFunctionCommunication->ReturnStatus = EFI_NOT_READY;
  SmmLightFunctionCommunication->Disable = TRUE;

  *CommunicationBuffer = SmmCommunicationHeader;

  if (DataPtr != NULL) {
    *DataPtr = (VOID *)SmmLightFunctionCommunication;
  }

  return EFI_SUCCESS;
}


/**
  Send the data in communicate buffer to SMI handler and get response.

  @param[in, out]  SmmCommunicateHeader    The communicate buffer.
  @param[in]       DataSize                The payload size.

**/
EFI_STATUS
SendCommunicationBufferLight (
  IN OUT  EFI_SMM_COMMUNICATE_HEADER        *SmmCommunicationHeader,
  IN      UINTN                               DataSize
  )
{
  EFI_STATUS                        Status;
  UINTN                             CommSize;
  SMM_LIGHT_FUNCTION_COMMUNICATION  *SmmFunctionHeader;

  CommSize = SMM_COMMUNICATE_HEADER_SIZE + DataSize;
  Status = mSmmCommunication->Communicate (mSmmCommunication, SmmCommunicationHeader, &CommSize);
    if (EFI_ERROR (Status)) {
    return Status;
  }
  SmmFunctionHeader = (SMM_LIGHT_FUNCTION_COMMUNICATION *) SmmCommunicationHeader->Data;

  return  SmmFunctionHeader->ReturnStatus;
}


/**
  OnExitBootServicesLight
  Unregister status code callback function - only available at boot time.

  @param  Event      Event whose notification function is being invoked.
  @param  Context    Pointer to the notification function's context.

**/
VOID
EFIAPI
OnExitBootServicesLight (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  EFI_SMM_COMMUNICATE_HEADER *SmmCommunicationHeader;
  EFI_STATUS Status = EFI_SUCCESS;
  UINTN DataSize = SMM_LIGHT_FUNCTION_COMMUNICATION_SIZE;

  //
  // Initialize communication buffer.
  //
  Status = InitializeCommBufferLight (
            (VOID **)&SmmCommunicationHeader, 
            NULL,
            UNREGISTER_FROM_RSC
            );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - InitializeCommBufferLight: %r\n", __FUNCTION__, Status));
    return;
  }

  //
  // Send data to SMM.
  //
  Status = SendCommunicationBufferLight (SmmCommunicationHeader, DataSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - SendCommunicationBufferLight: %r\n", __FUNCTION__, Status));
    return;
  }

  return;
}

/**
  DxeLight Module Entry Point

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       Initialization completed.
**/
EFI_STATUS
EFIAPI
DxeLightEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;
  EFI_EVENT                 ExitBootServiceEvent;

  mSmmCommunication = NULL;
  Status = gBS->LocateProtocol (
                  &gEfiSmmCommunicationProtocolGuid, 
                  NULL, 
                  (VOID **) &mSmmCommunication
                  );
  if (EFI_ERROR(Status)) {
    return Status;
  }

  mCommBuffer = AllocateRuntimeZeroPool(EFI_PAGES_TO_SIZE(1));
  if (mCommBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiLightStatusCodeCommunicationGuid,
                  mCommBuffer, 
                  NULL
                  );
  if (EFI_ERROR(Status)) {
    return Status;
  }

  gBS->CreateEventEx (
         EVT_NOTIFY_SIGNAL,
         TPL_NOTIFY,
         OnExitBootServicesLight,
         NULL,
         &gEfiEventExitBootServicesGuid,
         &ExitBootServiceEvent
         );
  return Status;
}