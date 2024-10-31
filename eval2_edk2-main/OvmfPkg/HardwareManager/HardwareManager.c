#include "HardwareManager.h"

THERMAL_SENSOR_PROTOCOL           *mThermalSensor;
FAN_CONTROLLER_PROTOCOL           *mFanController;
LED_CONTROLLER_PROTOCOL           *mLedController;

HARDWARE_MANAGER_LICENSE_PROTOCOL mHardwareManagerLicenseProtocol;

EFI_SMM_VARIABLE_PROTOCOL         *SmmVariableProtocol;
PROCESS_FIRMWARE_PROTOCOL         *mProcessFirmware;

UINT8 DefaultLicense[64] = {
  0x35, 0x64, 0xEA, 0xA3, 0xC1, 0x2C, 0x89, 0xC7, 0x49, 0xC6, 0x24, 0x8B, 0x6A, 0x08, 0x6C, 0xEA, 
  0xF6, 0xE6, 0x89, 0xDD, 0xF4, 0xC7, 0x06, 0x64, 0xC8, 0x35, 0xFA, 0xBC, 0xD0, 0x40, 0x71, 0x63, 
  0x8F, 0x53, 0xA2, 0xD8, 0xCC, 0xD1, 0x5B, 0xB3, 0x06, 0x64, 0x5D, 0x6B, 0x3D, 0x57, 0xF6, 0xDC, 
  0xDA, 0x92, 0xB2, 0xA8, 0xCD, 0xAA, 0x74, 0x99, 0x34, 0x54, 0x03, 0x61, 0xB1, 0xD0, 0xAD, 0xDE
};

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
HardwareManagerHandler (
  IN      EFI_HANDLE    DispatchHandle,
  IN      CONST VOID    *Context OPTIONAL,
  IN OUT  VOID          *CommBuffer OPTIONAL,
  IN OUT  UINTN         *CommBufferSize OPTIONAL
  )
{
  FIRMWARE_CHUNK                FirmwareChunk;
  FIRMWARE_CHUNK_METADATA       *FirmwarePointer;
  HARDWARE_MANAGER_COMM_BUFFER  *TempCommBuffer;

  if (CommBuffer == NULL || CommBufferSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FirmwarePointer = (FIRMWARE_CHUNK_METADATA *)FirmwareChunk;
  TempCommBuffer  = (HARDWARE_MANAGER_COMM_BUFFER *)CommBuffer;

  CopyMem(FirmwareChunk, (VOID *)TempCommBuffer->FirmwareChunk, METADATA_SIZE);
  
  //
  //
  // The FirmwareHead and FirmwareBody fields are passed as relative addresses. They need to be
  // adjusted to absolute addresses, and checked for boundaries.
 
  if ((UINTN)FirmwarePointer->FirmwareBody > sizeof(FIRMWARE_CONTENT)) {
    SetMem(FirmwarePointer, sizeof(FirmwareChunk), 0x00);
    TempCommBuffer->Status = EFI_INVALID_PARAMETER;
    return EFI_SUCCESS;
  }
  else if ((UINTN)FirmwarePointer->FirmwareHead > sizeof(FIRMWARE_CONTENT)) {
    SetMem(FirmwarePointer, sizeof(FirmwareChunk), 0x00);
    TempCommBuffer->Status = EFI_INVALID_PARAMETER;
    return EFI_SUCCESS;
  }

  FirmwarePointer->FirmwareBody = (VOID *)FirmwarePointer->Content + (UINTN)FirmwarePointer->FirmwareBody;
  FirmwarePointer->FirmwareHead = (VOID *)FirmwarePointer->Content + (UINTN)FirmwarePointer->FirmwareHead;
  
  //
  //
  // Copy the FirmwareChunk if it's required by FunctionCode
  if (TempCommBuffer->FunctionCode >= HW_FUNC_PF_BC_CHECK_FIRMWARE) {
    CopyMem(
      FirmwarePointer->LicenseKey,
      (TempCommBuffer->FirmwareChunk + METADATA_SIZE),
      sizeof(FIRMWARE_CHUNK_METADATA) - METADATA_SIZE
      );
  }
  
  TempCommBuffer->Status = EFI_SUCCESS;
  switch (TempCommBuffer->FunctionCode) {
    case HW_FUNC_LC_SET_LED_COLOR:
      TempCommBuffer->Status = mLedController->SetLedColor(
                                (UINT64) TempCommBuffer->Param1,
                                (UINT64) TempCommBuffer->Param2
                                );
      break;
    case HW_FUNC_LC_SET_LED_BRIGHTNESS:
      TempCommBuffer->Status = mLedController->SetLedBrightness(
                                (UINT64) TempCommBuffer->Param1,
                                (UINT64) TempCommBuffer->Param2,
                                (UINT64) TempCommBuffer->Param3
                                );
      break;
    case HW_FUNC_LC_SET_LED_EFFECT:
      TempCommBuffer->Status = mLedController->SetLedEffect(
                                (UINT64) TempCommBuffer->Param1,
                                (UINT64) TempCommBuffer->Param2,
                                (UINT64) TempCommBuffer->Param3
                                );
      break;
    case HW_FUNC_LC_RESET_LED:
      TempCommBuffer->Status = mLedController->ResetLed(
                              (UINT64) TempCommBuffer->Param1,
                              (UINT64) TempCommBuffer->Param2
                              );
      break;
    case HW_FUNC_TS_GET_TEMPERATURE:
      TempCommBuffer->Status = mThermalSensor->GetTemperature(
                              (UINT64)   TempCommBuffer->Param1,
                              (UINT64 *) &TempCommBuffer->Param2
                              );
      break;
    case HW_FUNC_TS_SET_THRESHOLD:
      TempCommBuffer->Status = mThermalSensor->SetThreshold(
                              (UINT64) TempCommBuffer->Param1,
                              (UINT64) TempCommBuffer->Param2
                              );
      break;
    case HW_FUNC_TS_GET_SENSOR_STATUS:
      TempCommBuffer->Status = mThermalSensor->GetSensorStatus(
                              (UINT64)   TempCommBuffer->Param1,
                              (UINT64 *) &TempCommBuffer->Param2
                              );
      break;
    case HW_FUNC_TS_RESET_SENSOR:
      TempCommBuffer->Status = mThermalSensor->ResetSensor(
                              (UINT64) TempCommBuffer->Param1,
                              (UINT64) TempCommBuffer->Param2
                              );
      break;
    case HW_FUNC_FC_SET_FAN_SPEED:
      TempCommBuffer->Status = mFanController->SetFanSpeed(
                              (UINT64) TempCommBuffer->Param1,
                              (UINT64) TempCommBuffer->Param2
                              );
      break;
    case HW_FUNC_FC_GET_FAN_SPEED:
      TempCommBuffer->Status = mFanController->GetFanSpeed(
                              (UINT64)   TempCommBuffer->Param1,
                              (UINT64 *) &TempCommBuffer->Param2
                              );
      break;
    case HW_FUNC_FC_SET_FAN_MODE:
      TempCommBuffer->Status = mFanController->SetFanMode(
                              (UINT64) TempCommBuffer->Param1,
                              (UINT64) TempCommBuffer->Param2
                              );
      break;
    case HW_FUNC_FC_GET_FAN_STATUS:
      TempCommBuffer->Status = mFanController->GetFanStatus(
                              (UINT64)   TempCommBuffer->Param1,
                              (UINT64)   TempCommBuffer->Param2,
                              (UINT64 *) &TempCommBuffer->Param3
                              );
      break;
    case HW_FUNC_PF_BC_CHECK_FIRMWARE:
      TempCommBuffer->Status = mProcessFirmware->BCCheckFirmware(
                              (UINT64 *) &TempCommBuffer->Param1
                              );
      break;
    case HW_FUNC_PF_BC_LOAD_FIRMWARE:
      TempCommBuffer->Status = mProcessFirmware->BCLoadFirmware(
                              (UINT64) TempCommBuffer->Param1,
                              (UINT64 *) &TempCommBuffer->Param2
                              );
      break;
    case HW_FUNC_PF_FC_CHECK_FIRMWARE:
      TempCommBuffer->Status = mProcessFirmware->FCCheckFirmware(
                              (UINT64 *) &TempCommBuffer->Param1
                              );
      break;
    case HW_FUNC_PF_FC_LOAD_FIRMWARE:
      TempCommBuffer->Status = mProcessFirmware->FCLoadFirmware(
                              (UINT64)   TempCommBuffer->Param1,
                              (UINT64 *) &TempCommBuffer->Param2
                              );
      break;
    case HW_FUNC_PF_LC_CHECK_FIRMWARE:
      TempCommBuffer->Status = mProcessFirmware->LCCheckFirmware(
                              (UINT64 *) &TempCommBuffer->Param1
                              );
      break;
    case HW_FUNC_PF_LC_LOAD_FIRMWARE:
      TempCommBuffer->Status = mProcessFirmware->LCLoadFirmware(
                              (UINT64)   TempCommBuffer->Param1,
                              (UINT64 *) &TempCommBuffer->Param2
                              );
      break;
    case HW_FUNC_PF_TS_CHECK_FIRMWARE:
      TempCommBuffer->Status = mProcessFirmware->TSCheckFirmware(
                              (UINT64 *) &TempCommBuffer->Param1
                              );
      break;
    case HW_FUNC_PF_TS_LOAD_FIRMWARE:
      TempCommBuffer->Status = mProcessFirmware->TSLoadFirmware(
                              (UINT64)   TempCommBuffer->Param1,
                              (UINT64 *) &TempCommBuffer->Param2
                              );
      break;
    case HW_FUNC_PF_VR_CHECK_FIRMWARE:
      TempCommBuffer->Status = mProcessFirmware->VRCheckFirmware(
                              (UINT64 *) &TempCommBuffer->Param1
                              );
      break;
    case HW_FUNC_PF_VR_LOAD_FIRMWARE:
      TempCommBuffer->Status = mProcessFirmware->VRLoadFirmware(
                              (UINT64)   TempCommBuffer->Param1,
                              (UINT64 *) &TempCommBuffer->Param2
                              );
      break;
    case HW_FUNC_HH_SET_LICENSE_KEY:
      TempCommBuffer->Status = HardwareManagerSetLicense(
                              (VOID *)FirmwarePointer->LicenseKey
                              );
      break;
    default:
      TempCommBuffer->Status = EFI_UNSUPPORTED;
      break;
  }

  SetMem(FirmwarePointer, sizeof(FirmwareChunk), 0x00);
  return EFI_SUCCESS;
}

/**
  This function sets this module's hardware license.

  @param[in] Data              The value of the new hardware license.
  
  @retval    EFI_SUCCESS       The hardware license was changed successfully.
  @retval    Others            Other errors as indicated.
**/
EFI_STATUS
HardwareManagerSetLicense(
  IN VOID *Data
  )
{
    EFI_STATUS  Status;
    UINT32      Attributes;
    UINTN       DataSize;
    
    DataSize    = 64;
    Attributes  = (EFI_VARIABLE_NON_VOLATILE |
                  EFI_VARIABLE_BOOTSERVICE_ACCESS |
                  EFI_VARIABLE_RUNTIME_ACCESS);

    //
    //
    // Set the UEFI variable
    Status = SmmVariableProtocol->SmmSetVariable(
              HARDWARE_HANDLER_LICENSE_VARIABLE,
              &gEfiHardwarHandlerVariableGuid,
              Attributes,
              DataSize,
              Data
              );
    return Status;
}

/**
  This function validates this module's hardware license. This is exposed in the Hardware Manager
  protocol.

  @param[in] License                The value of the new hardware license.
  
  @retval    EFI_SUCCESS            The hardware license is valid.
  @retval    EFI_INVALID_PARAMETER  The hardware license is invalid.
**/
EFI_STATUS
EFIAPI
HardwareManagerValidateLicense(
  IN   UINT8  *License
  )
{
  UINT32 i;
  UINT32 checksum = 0;
  
  for (i = 0; i < 60; ++i) {
    checksum += License[i];
  }

  checksum ^= LICENSE_MAGIC;
  UINT32 stored_checksum = License[60] |
                          (License[61] << 8) |
                          (License[62] << 16) |
                          (License[63] << 24);

  if (checksum != stored_checksum) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  This function retrieves this module's hardware license. This is exposed in the Hardware Manager
  protocol.

  @param[out] License                The location to which the license will be written.
  
  @retval     EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
HardwareManagerGetLicense(
  OUT   UINT8 *License
  )
{
    EFI_STATUS Status;
    UINTN DataSize;

    DataSize  = sizeof(DefaultLicense);
    Status    = SmmVariableProtocol->SmmGetVariable(
                HARDWARE_HANDLER_LICENSE_VARIABLE,
                &gEfiHardwarHandlerVariableGuid,
                NULL,
                &DataSize,
                License
                );
    if (EFI_ERROR(Status) || DataSize != sizeof(DefaultLicense)) {
      CopyMem((VOID *)License, (VOID *)DefaultLicense, sizeof(DefaultLicense));
      return EFI_SUCCESS;
    }
    
    return EFI_SUCCESS;
}

/**
  This protocol notify callback locates the Thermal Sensor, Fan Controller and Led Controller protocols
  after the Hardware Handler module has been loaded.

  @param[in] Protocol               The GUID of the notifying protocol.
  @param[in] Interface              The interface to which the protocol notification was bound.
  @param[in] Handle                 Points to an optional handler context which was specified when the
                                    handler was registered.
  
  @retval    EFI_SUCCESS            The protocols were located. 
  @retval    Others                 Other errors as indicated.
**/
EFI_STATUS
EFIAPI
HardwareHandlerReadyCallback (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS Status;

  Status = gSmst->SmmLocateProtocol(
                    &gEfiThermalSensorProtocolGuid,
                    NULL,
                    (VOID **)&mThermalSensor
                    );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "%a: Failed to locate ThermalSensorProtocol. Status: %r\n", __func__, Status));
    return Status;
  }

  Status = gSmst->SmmLocateProtocol(
                    &gEfiFanControllerProtocolGuid,
                    NULL,
                    (VOID **)&mFanController
                    );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "%a: Failed to locate FanControllerProtocol. Status: %r\n", __func__, Status));
    return Status;
  }

  Status = gSmst->SmmLocateProtocol(
                    &gEfiLedControllerProtocolGuid,
                    NULL,
                    (VOID **)&mLedController
                    );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "%a: Failed to locate LedControllerProtocol. Status: %r\n", __func__, Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  This protocol notify callback locates the Process Firmware protocol.

  @param[in] Protocol               The GUID of the notifying protocol.
  @param[in] Interface              The interface to which the protocol notification was bound.
  @param[in] Handle                 Points to an optional handler context which was specified when the
                                    handler was registered.
  
  @retval    EFI_SUCCESS            The protocol was located.
  @retval    Others                 Other errors as indicated.
**/
EFI_STATUS
EFIAPI
ProcessFirmwareReadyCallback (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS Status;

  Status = gSmst->SmmLocateProtocol(&gEfiProcessFirmwareProtocolGuid, NULL, (VOID **)&mProcessFirmware);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "%a: Failed to locate ProcessFirmwareProtocol. Status: %r\n", __func__, Status));
    return Status;
  }
  
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
HardwareManagerEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  EFI_HANDLE    DispatchHandle;
  EFI_HANDLE    ProtocolHandle;
  VOID          *ProcessFirmwareNotifyRegistration;
  VOID          *HardwareHandlerNotifyRegistration;

  mHardwareManagerLicenseProtocol.GetLicense       = NULL;
  mHardwareManagerLicenseProtocol.ValidateLicense  = NULL;

  //
  //
  // Load the protocols when HardwareHandler is ready
  Status = gMmst->MmRegisterProtocolNotify(
                    &gEfiHardwareHandlerReadyProtocolGuid,
                    HardwareHandlerReadyCallback,
                    &HardwareHandlerNotifyRegistration
                    );
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a: Could not register protocol notify\n", __func__));
    return Status;
  }

  //
  //
  // Load the protocols when ProcessFirmware is ready
  Status = gMmst->MmRegisterProtocolNotify(
                    &gEfiProcessFirmwareReadyProtocolGuid,
                    ProcessFirmwareReadyCallback,
                    &ProcessFirmwareNotifyRegistration
                    );
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a: Could not register protocol notify\n", __func__));
    return Status;
  }

  //
  //
  // Locate the SMM Variable Protocol
  Status = gSmst->SmmLocateProtocol(
            &gEfiSmmVariableProtocolGuid,
            NULL,
            (VOID **)&SmmVariableProtocol
            );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "%a: Could not locate SmmVariableProtocol !!!\n", __func__));  
    return Status;
  }

  //
  //
  // Install the hardware manager license protocol
  ProtocolHandle = NULL;
  mHardwareManagerLicenseProtocol.GetLicense       = HardwareManagerGetLicense;
  mHardwareManagerLicenseProtocol.ValidateLicense  = HardwareManagerValidateLicense;
  Status = gSmst->SmmInstallProtocolInterface(
            &ProtocolHandle,
            &gEfiHardwareLicenseProtocolGuid,
            EFI_NATIVE_INTERFACE,
            &mHardwareManagerLicenseProtocol
            );
  if (EFI_ERROR(Status)) {
    mHardwareManagerLicenseProtocol.GetLicense       = NULL;
    mHardwareManagerLicenseProtocol.ValidateLicense  = NULL;
    DEBUG((DEBUG_ERROR, "%a: Failed to install HardwareManagerLicenseProtocol\n", __func__));
    return Status;
  }

  //
  //
  // Register the MMI handler
  Status = gMmst->MmiHandlerRegister(
                    HardwareManagerHandler,
                    &gEfiHardwareManagerCommGuid,
                    &DispatchHandle
                    );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "%a: Failed to register DriverWriteHandler. Status: %r\n", __func__, Status));
    return Status;
  }

  return Status;
}
