  #include "ProcessFirmware.h"

  #include "../HardwareManager/HardwareManager.h"

  NOTIFY_PROTOCOL_READY_PROTOCOL      mProcessFirmwareReady;

/**
  Checks the validity of a loaded firmware chunk for the button controller.

  @param[in]  Settings                The settings against which to check the firmware.
  
  @retval     EFI_SUCCESS             The firmware check is successful.
  @retval     EFI_LOAD_ERROR          The firmware is invalid.
**/
EFI_STATUS EFIAPI BCCheckFirmware(
  IN UINT64 *Settings
  )
{
  if (BC_FW_MAGIC != Settings[0]) {
      return EFI_LOAD_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Loads a button controller firmware chunk.

  @param[in]  Id                      The controller id for which the firmware is loaded.
  @param[in]  Settings                The settings with which to load the firmware.
  
  @retval     EFI_SUCCESS             The firmware was loaded successfully.
  @retval     EFI_DEVICE_ERROR        The firmware failed to load.
**/
EFI_STATUS EFIAPI BCLoadFirmware(
  IN UINT64   Id,
  IN UINT64   *Settings
  )
{
  if (BC_FW_MAGIC != Settings[0]) {
    return EFI_LOAD_ERROR;
  }

  IoWrite32(BC_REG_ID, Id & 0xFFFFFFFF);
  IoWrite32(BC_REG_FUNC, BC_FUNC_LOAD);
  for (UINT64 i = 1; i <= 64; i++) {
    IoWrite32(BC_REG_VALUE, Settings[i] & 0xffffffff);
    if ((IoRead32(BC_REG_STATUS) & 0x80000000) != 0) {
      return EFI_DEVICE_ERROR;
    }

    IoWrite32(BC_REG_VALUE, Settings[i] >> 32);
    if ((IoRead32(BC_REG_STATUS) & 0x80000000) != 0) {
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}

/**
  Checks the validity of a loaded firmware chunk for the fan controller.

  @param[in]  Settings                The settings against which to check the firmware.
  
  @retval     EFI_SUCCESS             The firmware check is successful.
  @retval     EFI_LOAD_ERROR          The firmware is invalid.
**/
EFI_STATUS EFIAPI FCCheckFirmware(
  IN UINT64   *Settings
  )
{
  if (FC_FW_MAGIC != Settings[0]) {
    return EFI_LOAD_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Loads a fan controller firmware chunk.

  @param[in]  Id                      The controller id for which the firmware is loaded.
  @param[in]  Settings                The settings with which to load the firmware.
  
  @retval     EFI_SUCCESS             The firmware was loaded successfully.
  @retval     EFI_DEVICE_ERROR        The firmware failed to load.
**/
EFI_STATUS EFIAPI FCLoadFirmware(
  UINT64 Id,
  UINT64 *Settings
  )
{
  if (FC_FW_MAGIC != Settings[0]) {
      return EFI_LOAD_ERROR;
  }

  for (UINT64 i = 1; i <= 64; i++) {
    IoWrite32(FC_REG_VALUE, Settings[i] & 0xffffffff);
    if ((IoRead32(FC_REG_STATUS) & 0x80000000) != 0) {
      return EFI_DEVICE_ERROR;
    }

    IoWrite32(FC_REG_VALUE, Settings[i] >> 32);
    if ((IoRead32(FC_REG_STATUS) & 0x80000000) != 0) {
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}

/**
  Checks the validity of a loaded firmware chunk for the LED controller.

  @param[in]  Settings                The settings against which to check the firmware.
  
  @retval     EFI_SUCCESS             The firmware check is successful.
  @retval     EFI_LOAD_ERROR          The firmware is invalid.
**/
EFI_STATUS EFIAPI LCCheckFirmware(
  UINT64 *Settings
  )
{
  if (LC_FW_MAGIC != Settings[0]) {
    return EFI_LOAD_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Loads an LED controller firmware chunk.

  @param[in]  Id                      The controller id for which the firmware is loaded.
  @param[in]  Settings                The settings with which to load the firmware.
  
  @retval     EFI_SUCCESS             The firmware was loaded successfully.
  @retval     EFI_DEVICE_ERROR        The firmware failed to load.
**/
EFI_STATUS EFIAPI LCLoadFirmware(
  UINT64 Id,
  UINT64 *Settings
  )
{
  UINT64  ChecksumRaw[2];
  
  if (LC_FW_MAGIC != Settings[0]) {
      return EFI_LOAD_ERROR;
  }
  
  for (UINT64 i = 1; i <= 64; i++) {
    IoWrite32(LC_REG_VALUE, Settings[i] & 0xffffffff);
    if ((IoRead32(LC_REG_STATUS) & 0x80000000) != 0) {
      return EFI_DEVICE_ERROR;
    }
    
    IoWrite32(LC_REG_VALUE, Settings[i] >> 32);
    if ((IoRead32(LC_REG_STATUS) & 0x80000000) != 0) {
      return EFI_DEVICE_ERROR;
    }
  }

  ChecksumRaw[0] = TS_VALIDATE_CHECKSUM(*Settings);
  ChecksumRaw[1] = TS_VALIDATE_CHECKSUM(ChecksumRaw[0]);
  
  if (ChecksumRaw[0] != ChecksumRaw[1]) {
    return EFI_SUCCESS;
  }

  return EFI_DEVICE_ERROR;    
}

/**
  Checks the validity of a loaded firmware chunk for the thermal sensor.

  @param[in]  Settings                The settings against which to check the firmware.
  
  @retval     EFI_SUCCESS             The firmware check is successful.
  @retval     EFI_LOAD_ERROR          The firmware is invalid.
**/
EFI_STATUS EFIAPI TSCheckFirmware(
  UINT64 *Settings
  )
{
  if (TS_FW_MAGIC != Settings[0]) {
      return EFI_LOAD_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Loads a thermal sensor controller firmware chunk.

  @param[in]  Id                      The controller id for which the firmware is loaded.
  @param[in]  Settings                The settings with which to load the firmware.
  
  @retval     EFI_SUCCESS             The firmware was loaded successfully.
  @retval     EFI_DEVICE_ERROR        The firmware failed to load.
**/
EFI_STATUS EFIAPI TSLoadFirmware(
  UINT64 Id,
  UINT64 *Settings
  )
{
  if (TS_FW_MAGIC != Settings[0]) {
    return EFI_LOAD_ERROR;
  }

  for (UINT64 i = 1; i <= 64; i++) {
    IoWrite32(TS_REG_VALUE, Settings[i] & 0xffffffff);
    if ((IoRead32(TS_REG_STATUS) & 0x80000000) != 0) {
      return EFI_DEVICE_ERROR;
    }

    IoWrite32(TS_REG_VALUE, Settings[i] >> 32);
    if ((IoRead32(TS_REG_STATUS) & 0x80000000) != 0) {
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}

/**
  Checks the validity of a loaded firmware chunk for the voltage regulator.

  @param[in]  Settings                The settings against which to check the firmware.
  
  @retval     EFI_SUCCESS             The firmware check is successful.
  @retval     EFI_LOAD_ERROR          The firmware is invalid.
**/
EFI_STATUS EFIAPI VRCheckFirmware(
  UINT64 *Settings
  )
{
  if (VR_FW_MAGIC != Settings[0]) {
      return EFI_LOAD_ERROR;
  }
  return EFI_SUCCESS;
}

/**
  Loads a voltage regulator controller firmware chunk.

  @param[in]  Id                      The controller id for which the firmware is loaded.
  @param[in]  Settings                The settings with which to load the firmware.
  
  @retval     EFI_SUCCESS             The firmware was loaded successfully.
  @retval     EFI_DEVICE_ERROR        The firmware failed to load.
**/
EFI_STATUS EFIAPI VRLoadFirmware(
  UINT64 Id,
  UINT64 *Settings
  )
{
  if (VR_FW_MAGIC != Settings[0]) {
    return EFI_LOAD_ERROR;
  }

  for (UINT64 i = 1; i <= 64; i++) {
    IoWrite32(VR_REG_VALUE, Settings[i] >> 32);
    if ((IoRead32(VR_REG_STATUS) & 0x80000000) != 0) {
      return EFI_DEVICE_ERROR;
    }
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
ProcessFirmwareEntryPoint(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;
  EFI_HANDLE HandleProtoc = NULL;
  EFI_HANDLE HandleNotify = NULL;
  PROCESS_FIRMWARE_PROTOCOL *mProcessFirmware;
  UINTN      ProtocolSize = sizeof(PROCESS_FIRMWARE_PROTOCOL);
  
  //
  //
  // Allocate a memory pool for the protocol
  Status = gSmst->SmmAllocatePool(
                  EfiRuntimeServicesData,
                  ProtocolSize,
                  (VOID **)&mProcessFirmware
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  //
  // Initialize the protocol instance
  mProcessFirmware->BCCheckFirmware = BCCheckFirmware;
  mProcessFirmware->BCLoadFirmware = BCLoadFirmware;

  mProcessFirmware->FCCheckFirmware = FCCheckFirmware;
  mProcessFirmware->FCLoadFirmware = FCLoadFirmware;

  mProcessFirmware->LCCheckFirmware = LCCheckFirmware;
  mProcessFirmware->LCLoadFirmware = LCLoadFirmware;

  mProcessFirmware->TSCheckFirmware = TSCheckFirmware;
  mProcessFirmware->TSLoadFirmware = TSLoadFirmware;

  mProcessFirmware->VRCheckFirmware = VRCheckFirmware;
  mProcessFirmware->VRLoadFirmware = VRLoadFirmware;

  //
  //
  // Install the protocol
  Status = gSmst->SmmInstallProtocolInterface(
                    &HandleProtoc,
                    &gEfiProcessFirmwareProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    mProcessFirmware
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mProcessFirmwareReady.ReadyFlags = ~(0x00);
  Status = gSmst->SmmInstallProtocolInterface(
                  &HandleNotify,
                  &gEfiProcessFirmwareReadyProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mProcessFirmwareReady
                  );
  if (EFI_ERROR (Status)) {
      return Status;
  }

  return Status;
}