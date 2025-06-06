/** @file
  This file implements the Smm Light Control.
  Receive input from Report Status Code to change Lights.

**/

#include "SmmLight.h"

EFI_MM_RSC_HANDLER_PROTOCOL    *mRscProtocol           = NULL;
EFI_MM_FRONTPANEL_LED_PROTOCOL *mFplProtocol           = NULL;
EFI_EVENT                      mExitBootServicesEvent  = NULL;
VOID                           *mCommBuffer            = NULL;

/**

 Function to handle Light Status Code.

 @param  CodeType    Status code reported - UINT32.
 @param  Value       Value for the status code - UINT32.
 @param  Instance    Optional instance (count) associated with status code and value.
 @param  CallerId    Optional GUID.
 @param  Data        Optional additional data.


 @retval EFI_SUCCESS        Function completes successfully.
         EFI_UNSUPPORTED    Invalid CodeType.
*/
EFI_STATUS
EFIAPI
LightStatusCode (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId,
  IN EFI_STATUS_CODE_DATA     *Data
  )
{

  if (CodeType > MAX_LIGHT_TYPE || Instance == 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Get Front Panel LED protocol
  // 
  EFI_STATUS Status = EFI_SUCCESS;

  Status = gMmst->MmLocateProtocol (
                    &gEfiMmFrontPanelLedProtocolGuid,
                    NULL,
                    (VOID **) &mFplProtocol
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  mFplProtocol->Index    = Instance;
  mFplProtocol->Function = CodeType;
  mFplProtocol->Duration = Value;

  return mFplProtocol->StatusUpdate(mFplProtocol);
}

/**
  Notification for Report Status Code Protocol.

  @param[in] Protocol     Points to the protocol's unique identifier.
  @param[in] Interface    Points to the interface instance.
  @param[in] Handle       The handle on which the interface was installed.

  @retval EFI_SUCCESS    Notification handler runs successfully.
  @retval other          Some error occurs.

**/
EFI_STATUS
EFIAPI
RscAvailable (
  IN CONST EFI_GUID    *Protocol,
  IN VOID              *Interface,
  IN EFI_HANDLE        Handle
  )
{
  EFI_STATUS Status = EFI_SUCCESS;

  Status = gMmst->MmLocateProtocol (
                    &gEfiMmRscHandlerProtocolGuid,
                    NULL,
                    (VOID **) &mRscProtocol
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Register with ReportStatusCode
  //
  Status = mRscProtocol->Register (LightStatusCode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return Status;
}


/**
  SMI function to unregister Light Status Code function.

  Caution: This function may receive untrusted input.
  Communicate buffer and buffer size are external input, so this function will do basic validation.

  @param DispatchHandle    The unique handle assigned to this handler by MmiHandlerRegister().
  @param Context           Points to an optional handler context which was specified when the
                           handler was registered.
  @param CommBuffer        A pointer to a collection of data in memory that will
                           be conveyed from a non-SMM environment into an SMM environment.
  @param CommBufferSize    The size of the CommBuffer.

  @retval EFI_SUCCESS    Command is handled successfully.
**/
EFI_STATUS
EFIAPI
UnregisterCBFunctionLight (
  IN EFI_HANDLE    DispatchHandle,
  IN CONST VOID    *Context         OPTIONAL,
  IN OUT VOID      *CommBuffer      OPTIONAL,
  IN OUT UINTN     *CommBufferSize  OPTIONAL
  )
{
  SMM_LIGHT_FUNCTION_COMMUNICATION *CommunicationData;
  EFI_STATUS Status = EFI_UNSUPPORTED;

  //
  // If input is invalid, stop processing this SMI.
  //
  if (CommBuffer == NULL || CommBufferSize == NULL) {
    return EFI_SUCCESS;
  }

  if (*CommBufferSize != SMM_LIGHT_FUNCTION_COMMUNICATION_SIZE) {
    return EFI_SUCCESS;
  }

  if ((UINTN)CommBuffer != (UINTN)mCommBuffer + SMM_COMMUNICATE_HEADER_SIZE) {
    return EFI_SUCCESS;
  }

  CommunicationData = (SMM_LIGHT_FUNCTION_COMMUNICATION *)CommBuffer;
  switch (CommunicationData->Function) {
    case UNREGISTER_FROM_RSC:
      if (CommunicationData->Disable == FALSE)
        break;

      //
      // Unregister from Report Status Code.
      //
      if (mRscProtocol != NULL) {
        Status = mRscProtocol->Unregister (LightStatusCode);
        if (EFI_ERROR (Status)) {
          DEBUG((DEBUG_ERROR, "%a - Unregister LightStatusCode: %r\n", __FUNCTION__, Status));
        }
      }
      CommunicationData->ReturnStatus = Status;
    break;

    default :
      CommunicationData->ReturnStatus = EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}


/**
  SmmLight Module Entry Point
  Dxe/Smm Light driver handles all light notification events during Dxe.

 @param  ImageHandle       The firmware allocated handle for the EFI image.
 @param  SystemTable       A pointer to the EFI System Table.

 @retval EFI Status       Initialization completed.
**/
EFI_STATUS
EFIAPI
SmmLightEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS                Status;
  EFI_EVENT                 RscHandlerEvent;
  EFI_HANDLE                DispatchHandle;

  //
  // Locate Report Status Code Protocol
  //
  Status = gMmst->MmLocateProtocol (
                    &gEfiMmRscHandlerProtocolGuid,
                    NULL,
                    (VOID **) &mRscProtocol
                    );
  if (EFI_ERROR (Status)) {
    //
    // RSC handler not available, try again when it is.
    //
    Status = gMmst->MmRegisterProtocolNotify (
                      &gEfiMmRscHandlerProtocolGuid,
                      RscAvailable,
                      &RscHandlerEvent
                      );
  } else {
    //
    // Register with Report Status Code driver.
    //
    Status = mRscProtocol->Register (LightStatusCode);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Register LightStatusCode function: %r\n", __FUNCTION__, Status));
      return Status;
    }
  }

  //
  // Smi to unregister after exiting Boot Service.
  //
  Status = gMmst->MmiHandlerRegister (
                    UnregisterCBFunctionLight,
                    &gEfiLightStatusCodeCommunicationGuid,
                    &DispatchHandle
                    );
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "Register MMI UnregisterCBFunctionLight: %r\n", Status));
  }

  //
  // Dxe Light Status Protocol for Communication Buffer
  //
  Status = gBS->LocateProtocol (
                  &gEfiLightStatusCodeCommunicationGuid,
                  NULL,
                  (VOID **) &mCommBuffer
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Locate Protocol: %r\n", __FUNCTION__, Status));
  }
  return Status;
}
