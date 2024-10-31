#include <Protocol/SmmVariable.h>

#include "FanController.h"
#include "ThermalSensor.h"
#include "LEDController.h"

#include "../HardwareManager/HardwareManager.h"

extern EFI_GUID gEfiHardwarHandlerVariableGuid;
extern EFI_GUID gEfiHardwareHandlerReadyProtocolGuid;

#define LICENSE_MAGIC   0xdeadf00d
#define LICENSE_LENGTH  64

#define HARDWARE_HANDLER_LICENSE_VARIABLE L"HardwareHandlerLicense"

HARDWARE_MANAGER_LICENSE_PROTOCOL   *mHardwareManagerLicenseProtocol;
NOTIFY_PROTOCOL_READY_PROTOCOL      mHardwareHandlerReady;

/**
  This function retrieves the state of a button.

  @param[in]  ButtonId               The Id of the button.
  @param[in]  Flags                  The location to which the state is written.
  
  @retval     EFI_SUCCESS            The button's state was retrieved.
  @retval     EFI_DEVICE_ERROR       The button device failed.
**/
EFI_STATUS
EFIAPI
GetButtonState (
  IN  UINTN   ButtonId,
  OUT UINT64  *State
  )
{
  UINT32 Status;  
  IoWrite32(BC_REG_ID, ButtonId);
  IoWrite32(BC_REG_FUNC, BC_FUNC_GET_STATE);

  Status = IoRead32(BC_REG_VALUE);
  if (!(Status & BC_STATUS_SUCCESS)) {
    return EFI_DEVICE_ERROR;
  }

  *State = Status & BC_STATUS_RESULT;
  return EFI_SUCCESS;
}

/**
  This function locks a button.

  @param[in]  ButtonId               The Id of the button.
  @param[in]  Flags                  The lock state to be applied.
  
  @retval     EFI_SUCCESS            The button locked.
  @retval     EFI_DEVICE_ERROR       The button lock failed.
**/
EFI_STATUS
EFIAPI
SetButtonLock (
  IN UINTN    ButtonId,
  IN UINT64   LockState
  )
{
  UINT32 Status;
  IoWrite32(BC_REG_ID, ButtonId);
  IoWrite32(BC_REG_FUNC, BC_FUNC_SET_LOCK);
  IoWrite32(BC_REG_VALUE, (UINT32)(LockState & 0xFFFFFFFF));
  Status = IoRead32(BC_REG_VALUE);
  return (Status & BC_STATUS_SUCCESS) ? EFI_SUCCESS : EFI_DEVICE_ERROR;
}

/**
  This function triggers a button action.

  @param[in]  ButtonId               The Id of the button.
  @param[in]  Flags                  The action flags to be applied.
  
  @retval     EFI_SUCCESS            The button action was triggered.
  @retval     EFI_DEVICE_ERROR       The button action failed.
**/
EFI_STATUS
EFIAPI
TriggerButtonAction (
  IN UINTN    ButtonId,
  IN UINT64   Flags
  )
{
  UINT32 Status;
  IoWrite32(BC_REG_ID, ButtonId);
  IoWrite32(BC_REG_FUNC, BC_FUNC_TRIGGER);
  IoWrite32(BC_REG_VALUE, (UINT32)(Flags & 0xFFFFFFFF));
  Status = IoRead32(BC_REG_VALUE);
  return (Status & BC_STATUS_SUCCESS) ? EFI_SUCCESS : EFI_DEVICE_ERROR;
}

/**
  This function resets a button's active state.

  @param[in]  ButtonId               The Id of the button.
  @param[in]  Flags                  A mask which filters the current button's status bits.
  
  @retval     EFI_SUCCESS            The button was reset.
  @retval     EFI_DEVICE_ERROR       The button could not be reset.
**/
EFI_STATUS
EFIAPI
ResetButtonState (
  IN UINTN ButtonId,
  IN UINT64 Flags
  )
{
  UINT32  Status;
  IoWrite32(BC_REG_ID, ButtonId);
  IoWrite32(BC_REG_FUNC, BC_FUNC_RESET);
  IoWrite32(BC_REG_VALUE, (UINT32)(Flags & 0xFFFFFFFF));
  Status = IoRead32(BC_REG_VALUE);
  return (Status & BC_STATUS_SUCCESS) ? EFI_SUCCESS : EFI_DEVICE_ERROR;
}

/**
  This function sets the speed of a fan.

  @param[in]  FanId                  The Id of the fan.
  @param[in]  Speed                  The desired speed of the fan.
  
  @retval     EFI_SUCCESS            The fan's speed was written.
  @retval     EFI_DEVICE_ERROR       The fan's speed was not written.
**/
EFI_STATUS
EFIAPI
SetFanSpeed (
  IN UINTN  FanId,
  IN UINTN  Speed
  )
{
  IoWrite32(FC_REG_ID, FanId & 0Xffffffff);
  IoWrite32(FC_REG_FUNC, FC_SET_SPEED);
  IoWrite32(FC_REG_VALUE, Speed & 0xffffffff);
  UINT32 Status = IoRead32(FC_REG_STATUS);

  if (Status & 0x80000000) {
    return EFI_SUCCESS;
  }
  
  return EFI_DEVICE_ERROR;
}

/**
  This function gets the speed of a fan.

  @param[in]  FanId                  The Id of the fan.
  @param[in]  Speed                  The location to which the speed is written.
  
  @retval     EFI_SUCCESS            The fan's speed was read.
  @retval     EFI_NO_RESPONSE        The fan's speed was not read.
**/
EFI_STATUS
EFIAPI
GetFanSpeed (
  IN UINTN FanId,
  OUT UINTN *Speed
  )
{
  UINTN Result  = 0;
  IoWrite32(FC_REG_ID, FanId & 0xffffffff);
  IoWrite32(FC_REG_FUNC, FC_GET_SPEED);
  Result = IoRead32(FC_REG_VALUE);

  if (Result & 0x80000000) {
    return EFI_NO_RESPONSE;
  }

  *Speed = Result & 0x7fffffff;
  return EFI_SUCCESS;
}

/**
  This function set a fan's mode.

  @param[in]  FanId                  The Id of the fan.
  @param[in]  Flags                  The mode to which the fan is set.
  
  @retval     EFI_SUCCESS            The fan's mode has been written.
  @retval     EFI_DEVICE_ERROR       The fan device has failed.
**/
EFI_STATUS
EFIAPI
SetFanMode (
  IN UINTN    FanId,
  IN UINT64   Mode
  )
{
  UINT32  Status;
  IoWrite32(FC_REG_ID, FanId & 0Xffffffff);
  IoWrite32(FC_REG_FUNC, FC_SET_MODE);
  IoWrite32(FC_REG_VALUE, Mode & 0xffffffff);
  Status = IoRead32(FC_REG_STATUS);

  if (Status & 0x80000000) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  This function gets the status of a fan.

  @param[in]  FanId                  The Id of the fan.
  @param[in]  Flags                  Flags to apply when reading the fan's status.
  @param[out] Value                  The address to which to write the fan's status.
  
  @retval     EFI_SUCCESS            The fan's status has been read.
  @retval     EFI_DEVICE_ERROR       The fan device has failed.
**/
EFI_STATUS
EFIAPI
GetFanStatus (
  IN  UINTN    FanId,
  IN  UINT64   Flags,
  OUT UINTN    *Value
  )
{
  IoWrite32(FC_REG_ID, FanId & 0xffffffff);
  IoWrite32(FC_REG_FUNC, FC_GET_STATUS);
  IoWrite32(FC_REG_VALUE, Flags & 0xffffffff);
  *Value = IoRead32(FC_REG_VALUE);

  if ((*Value & 0x7fffffff) == 0) {
    return EFI_NOT_READY;
  }

  return EFI_SUCCESS;
}

/**
  This function sets the color of an LED.

  @param[in]  LedId                  The Id of the LED.
  @param[in]  RGB                    The color value to be applied to the LED.
  
  @retval     EFI_SUCCESS            The color has been applied to the LED.
  @retval     EFI_DEVICE_ERROR       The LED device has failed.
**/
EFI_STATUS
EFIAPI
SetLedColor (
  IN UINTN    LedId,
  IN UINT64   RGB
  )
{
  UINT32 ColorValue = (RGB) & 0xffffffff;
  IoWrite32(LC_REG_ID, LedId & 0xffffffff);
  IoWrite32(LC_REG_FUNC, LC_COLOR);
  IoWrite32(LC_REG_VALUE, ColorValue);
  UINT32 Status = IoRead32(LC_REG_STATUS);

  if (Status == 0x00) {
    return EFI_SUCCESS;
  }
  
  return EFI_DEVICE_ERROR;
}

/**
  This function sets the brightness of an LED.

  @param[in]  LedId                  The Id of the LED.
  @param[in]  Brightness             The brightness value to be applied to the LED.
  @param[in]  Flags                  Flags to apply to the LED's brightness bits.
  
  @retval     EFI_SUCCESS            The brightness has been applied to the LED.
  @retval     EFI_DEVICE_ERROR       The LED device has failed.
**/
EFI_STATUS
EFIAPI
SetLedBrightness (
  IN UINTN    LedId,
  IN UINT64   Brightness,
  IN UINT64   Flags
  )
{
  IoWrite32(LC_REG_ID, LedId & 0xffffffff);
  IoWrite32(LC_REG_FUNC, (LC_BRIGHTNESS|Flags) & 0xffffffff);
  IoWrite32(LC_REG_VALUE, (UINT32)(Brightness & 0xffffffff));
  UINT32 Status = IoRead32(LC_REG_STATUS);

  if (Status == 0x0) {
    return EFI_SUCCESS;
  }
  
  return EFI_DEVICE_ERROR;
}

/**
  This function sets an effect on an LED.

  @param[in]  LedId                  The Id of the LED to which the effect is applied.
  @param[in]  EffectType             The type of effect being applied.
  @param[in]  Speed                  The speed at which the effect is applied.
  
  @retval     EFI_SUCCESS            The effect has been applied to the LED.
  @retval     EFI_DEVICE_ERROR       The LED device has failed.
**/
EFI_STATUS
EFIAPI
SetLedEffect (
  IN UINTN    LedId,
  IN UINT64   EffectType,
  IN UINT64   Speed
  )
{
  UINT32 Value = ((EffectType & 0xffff) | ((Speed<<16) & 0xffff0000)) & 0xffffffff;
  IoWrite32(LC_REG_ID, LedId & 0xffffffff);
  IoWrite32(LC_REG_FUNC, LC_EFFECT);
  IoWrite32(LC_REG_VALUE, Value);
  UINT32 Status = IoRead32(LC_REG_STATUS);

  if (Status == 0x0) {
    return EFI_SUCCESS;
  }

  return EFI_DEVICE_ERROR;
}

/**
  This function resets the an LED.

  @param[in]  LedId                  The Id of the LED being reset.
  @param[in]  Flags                  Not implemented yet.
  
  @retval    EFI_SUCCESS             The LED has been reset.
  @retval    EFI_DEVICE_ERROR        The device failed.
**/
EFI_STATUS
EFIAPI
ResetLed (
  IN UINTN    LedId,
  IN UINT64   Flags
  )
{
  IoWrite32(LC_REG_ID, LedId & 0xffffffff);
  IoWrite32(LC_REG_FUNC, LC_RESET);
  IoWrite32(LC_REG_VALUE, LC_RESET);
  UINT32 Status = IoRead32(LC_REG_STATUS);

  if (Status == 0x0) {
    return EFI_SUCCESS;
  }

  return EFI_DEVICE_ERROR;
}

/**
  This function retrieves the current read of a thermal sensor.

  @param[in]  SensorId               The Id of the sensor being read.
  @param[in]  Threshold              The location to which to write the read.
  
  @retval    EFI_SUCCESS             The sensor was successfully read.
**/
EFI_STATUS
EFIAPI
GetTemperature (
  IN  UINTN   SensorId,
  OUT UINTN   *Temperature
  )
{
  IoWrite32(TS_REG_ID, SensorId & 0xffffffff);
  IoWrite32(TS_REG_FUNC, TS_GET_TEMPERATURE & 0xffffffff);
  *Temperature = IoRead32(TS_REG_VALUE);
  
  return EFI_SUCCESS;
}

/**
  This function sets the alarm threshold of a thermal sensor.

  @param[in]  SensorId               The Id of the sensor being reset.
  @param[in]  Threshold              The sensor's new alarm threshold.
  
  @retval    EFI_SUCCESS             The sensor's threshold has be set.
**/
EFI_STATUS
EFIAPI
SetThreshold (
  IN UINTN    SensorId,
  IN UINTN    Threshold
  )
{
  IoWrite32(TS_REG_ID, SensorId & 0Xffffffff);
  IoWrite32(TS_REG_FUNC, TS_SET_THRESHOLD & 0xffffffff);
  IoWrite32(TS_REG_VALUE, Threshold & 0xffffffff);
  return EFI_SUCCESS;
}

/**
  This function retrieves the status bits of a thermal sensor.

  @param[in]  SensorId               The Id of the sensor being reset.
  @param[out] ValMask                The location to which the sensor status should be written.
  
  @retval    EFI_SUCCESS             The sensor is active.
  @retval    EFI_NOT_AVAILABLE_YET   The sensor is not available.
**/
EFI_STATUS
EFIAPI
GetSensorStatus (
  IN  UINTN     SensorId,
  IN  UINT64    *ValMask
  )
{
  BOOLEAN IsActive;
  IoWrite32(TS_REG_ID, SensorId & 0Xffffffff);
  IoWrite32(TS_REG_FUNC, TS_GET_SENSOR_STATUS & 0xffffffff);
  *ValMask = IoRead32(TS_REG_VALUE) & 0xffffffff;
  IsActive = ((*ValMask & 0xff) == 0);
  return IsActive ? EFI_SUCCESS : EFI_NOT_AVAILABLE_YET;
}

/**
  This function resets the status bits of a thermal sensor.

  @param[in] SensorId               The Id of the sensor being reset.
  @param[in] StartMask              A mask which filters the current sensor status bits.
  
  @retval    EFI_SUCCESS            The sensor was reset.
**/
EFI_STATUS
EFIAPI
ResetSensor (
  IN UINTN    SensorId,
  IN UINT64   ValMask
  )
{
  IoWrite32(TS_REG_ID, SensorId & 0Xffffffff);
  IoWrite32(TS_REG_FUNC, TS_RESET_SENSOR & 0xffffffff);
  IoWrite32(TS_REG_VALUE, IoRead32(TS_REG_VALUE) & ValMask);
  return EFI_SUCCESS;
}

/**
  This function uses the HardwareManagerLicense protocol to retrieve and validate the system's
  hardware license.
  
  @retval    EFI_SUCCESS            The license was validated.
  @retval    EFI_INVALID_PARAMETER  The license could not be validated.
**/
EFI_STATUS HardwareHandlerCheckLicense(
  )
{
  EFI_STATUS Status;
  UINT8 Data[64] = {0};

  Status = mHardwareManagerLicenseProtocol->GetLicense(Data);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a: Cannot retrieve legitimate hardware license !!!\n", __func__));
    return Status;
  }

  Status = mHardwareManagerLicenseProtocol->ValidateLicense(Data);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_INFO, "%a: Invalid hardware license\n", __func__));
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
HardwareHandlerEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  ) 
{
  EFI_STATUS Status;
  FAN_CONTROLLER_PROTOCOL *mFanControllerProtocol;
  THERMAL_SENSOR_PROTOCOL *mThermalSensorProtocol;
  LED_CONTROLLER_PROTOCOL *mLedControllerProtocol;
  EFI_HANDLE              Handle;
  
  Status = gSmst->SmmLocateProtocol(
              &gEfiHardwareLicenseProtocolGuid,
              NULL,
              (VOID **)&mHardwareManagerLicenseProtocol
              );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  //
  // Allocate, define, and install the FanController protocol

  Status = gSmst->SmmAllocatePool(
                    EfiRuntimeServicesData,
                    sizeof(FAN_CONTROLLER_PROTOCOL),
                    (VOID **)&mFanControllerProtocol
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mFanControllerProtocol->SetFanSpeed   = SetFanSpeed;
  mFanControllerProtocol->GetFanSpeed   = GetFanSpeed;
  mFanControllerProtocol->SetFanMode    = SetFanMode;
  mFanControllerProtocol->GetFanStatus  = GetFanStatus;

  Handle = NULL;
  Status = gSmst->SmmInstallProtocolInterface(
                    &Handle,
                    &gEfiFanControllerProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    mFanControllerProtocol
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  //
  // Allocate, define, and install the LedController protocol
  
  Status = gSmst->SmmAllocatePool(
                    EfiRuntimeServicesData,
                    sizeof(LED_CONTROLLER_PROTOCOL),
                    (VOID **)&mLedControllerProtocol
                    );
  if (EFI_ERROR (Status)) {
      return Status;
  }

  mLedControllerProtocol->SetLedColor       = SetLedColor;
  mLedControllerProtocol->SetLedBrightness  = SetLedBrightness;
  mLedControllerProtocol->SetLedEffect      = SetLedEffect;
  mLedControllerProtocol->ResetLed          = ResetLed;

  Handle = NULL;
  Status = gSmst->SmmInstallProtocolInterface(
                    &Handle,
                    &gEfiLedControllerProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    mLedControllerProtocol
                    );
  if (EFI_ERROR (Status)) {
      return Status;
  }

  //
  //
  // Allocate, define, and install the ThermalSensor protocol

  Status = gSmst->SmmAllocatePool(
                    EfiRuntimeServicesData,
                    sizeof(THERMAL_SENSOR_PROTOCOL),
                    (VOID **)&mThermalSensorProtocol
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mThermalSensorProtocol->GetTemperature  = GetTemperature;
  mThermalSensorProtocol->SetThreshold    = SetThreshold;
  mThermalSensorProtocol->GetSensorStatus = GetSensorStatus;
  mThermalSensorProtocol->ResetSensor     = ResetSensor;

  
  Handle = NULL;
  Status = gSmst->SmmInstallProtocolInterface(
                    &Handle,
                    &gEfiThermalSensorProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    mThermalSensorProtocol
                    );
  if (EFI_ERROR (Status)) {
      return Status;
  }

  //
  //
  // Install the HardwareHandlerReady protocol

  Handle = NULL;
  mHardwareHandlerReady.ReadyFlags = ~(0x00);
  Status = gSmst->SmmInstallProtocolInterface(
            &Handle,
            &gEfiHardwareHandlerReadyProtocolGuid,
            EFI_NATIVE_INTERFACE,
            &mHardwareHandlerReady
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  //
  // Validate the system's hardware license

  Status = HardwareHandlerCheckLicense();
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}
