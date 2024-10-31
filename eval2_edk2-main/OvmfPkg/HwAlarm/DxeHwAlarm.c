/** @file
  This file implements the Dxe HwAlarm Control.
  Disable HwAlarm SMM at ExitBootServices.

**/

#include "SmmHwAlarm.h"

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
InitializeCommBufferHwAlarm (
  OUT  VOID     **CommunicationBuffer,
  OUT  VOID     **DataPtr,
  IN   UINTN    Function
  )
{
  EFI_SMM_COMMUNICATE_HEADER        *SmmCommunicationHeader;
  SMM_HWALARM_FUNCTION_COMMUNICATION  *SmmHwAlarmFunctionCommunication;
  UINTN                             SmmCommunicationSize;

  //
  //
  // Init the communicate buffer. The buffer data size is:
  // SMM_COMMUNICATE_HEADER_SIZE + SMM_HWALARM_FUNCTION_COMMUNICATION_SIZE.
  SmmCommunicationHeader  = mCommBuffer;
  SmmCommunicationSize    = SMM_COMMUNICATE_HEADER_SIZE + SMM_HWALARM_FUNCTION_COMMUNICATION_SIZE;
  if (SmmCommunicationSize > EFI_PAGES_TO_SIZE (1)) {
    return EFI_ACCESS_DENIED;
  }

  ZeroMem (SmmCommunicationHeader, SmmCommunicationSize);

  //
  //
  // Setup Communication Buffer Data.
  CopyGuid (&SmmCommunicationHeader->HeaderGuid, &gEfiHwAlarmStatusCodeCommGuid);

  SmmCommunicationHeader->MessageLength = SMM_HWALARM_FUNCTION_COMMUNICATION_SIZE;
  SmmHwAlarmFunctionCommunication       = (SMM_HWALARM_FUNCTION_COMMUNICATION *) SmmCommunicationHeader->Data;

  SmmHwAlarmFunctionCommunication->Function     = Function;
  SmmHwAlarmFunctionCommunication->ReturnStatus = EFI_NOT_READY;
  SmmHwAlarmFunctionCommunication->Disable      = TRUE;

  *CommunicationBuffer = SmmCommunicationHeader;

  if (DataPtr != NULL) {
    *DataPtr = (VOID *)SmmHwAlarmFunctionCommunication;
  }

  return EFI_SUCCESS;
}


/**
  Send the data in communicate buffer to SMI handler and get response.

  @param[in, out]  SmmCommunicateHeader    The communicate buffer.
  @param[in]       DataSize                The payload size.

**/
EFI_STATUS
SendCommunicationBufferHwAlarm (
  IN OUT  EFI_SMM_COMMUNICATE_HEADER        *SmmCommunicationHeader,
  IN      UINTN                               DataSize
  )
{
  EFI_STATUS                        Status;
  UINTN                             CommSize;
  SMM_HWALARM_FUNCTION_COMMUNICATION  *SmmFunctionHeader;

  CommSize = SMM_COMMUNICATE_HEADER_SIZE + DataSize;
  Status = mSmmCommunication->Communicate (mSmmCommunication, SmmCommunicationHeader, &CommSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SmmFunctionHeader = (SMM_HWALARM_FUNCTION_COMMUNICATION *) SmmCommunicationHeader->Data;
  return  SmmFunctionHeader->ReturnStatus;
}


/**
  OnExitBootServicesHwAlarm
  Unregister status code callback function - only available at boot time.

  @param  Event      Event whose notification function is being invoked.
  @param  Context    Pointer to the notification function's context.

**/
VOID
EFIAPI
OnExitBootServicesHwAlarm (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  EFI_SMM_COMMUNICATE_HEADER *SmmCommunicationHeader;
  EFI_STATUS Status = EFI_SUCCESS;
  UINTN DataSize = SMM_HWALARM_FUNCTION_COMMUNICATION_SIZE;

  //
  //
  // Initialize communication buffer.
  Status = InitializeCommBufferHwAlarm (
            (VOID **)&SmmCommunicationHeader, 
            NULL,
            UNREGISTER_FROM_RSC
            );
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a - InitializeCommBufferHwAlarm: %r\n", __FUNCTION__, Status));
    return;
  }

  //
  //
  // Send data to SMM.
  Status = SendCommunicationBufferHwAlarm (SmmCommunicationHeader, DataSize);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a - SendCommunicationBufferHwAlarm: %r\n", __FUNCTION__, Status));
    return;
  }

  return;
}

/**
  DxeHwAlarm Module Entry Point

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       Initialization completed.
**/
EFI_STATUS
EFIAPI
DxeHwAlarmEntryPoint (
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
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mCommBuffer = AllocateRuntimeZeroPool(EFI_PAGES_TO_SIZE(1));
  if (mCommBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiHwAlarmStatusCodeCommGuid,
                  mCommBuffer, 
                  NULL
                  );
  if (EFI_ERROR(Status)) {
    return Status;
  }

  gBS->CreateEventEx (
         EVT_NOTIFY_SIGNAL,
         TPL_NOTIFY,
         OnExitBootServicesHwAlarm,
         NULL,
         &gEfiEventExitBootServicesGuid,
         &ExitBootServiceEvent
         );

  return Status;
}