/**************************************************************
 * InstrumentedHardwareHandler.c
 *
 * Your existing HardwareHandler code is kept as-is, 
 * just placed in one file for clarity.
 **************************************************************/

#include "../../eval2_edk2-main/MdeModulePkg/Include/Protocol/SmmVariable.h"
#include "../../eval2_edk2-main/OvmfPkg/HardwareHandler/FanController.h"
#include "../../eval2_edk2-main/OvmfPkg/HardwareHandler/ThermalSensor.h"
#include "../../eval2_edk2-main/OvmfPkg/HardwareHandler/LEDController.h"

#include "../../eval2_edk2-main/OvmfPkg/HardwareManager/HardwareManager.h"

/* If you'd like to use KLEE for possible asserts or instrumentation, 
   you can do something like:
#ifdef USE_KLEE
#include "klee/klee.h"
#else
#define klee_assert(expr) /* no-op 
#endif
*/

extern EFI_GUID gEfiHardwarHandlerVariableGuid;
extern EFI_GUID gEfiHardwareHandlerReadyProtocolGuid;

#define LICENSE_MAGIC   0xdeadf00d
#define LICENSE_LENGTH  64

#define HARDWARE_HANDLER_LICENSE_VARIABLE L"HardwareHandlerLicense"

HARDWARE_MANAGER_LICENSE_PROTOCOL   *mHardwareManagerLicenseProtocol;
NOTIFY_PROTOCOL_READY_PROTOCOL      mHardwareHandlerReady;

// --------------------------------------------------------------------------
//  (Your existing function definitions follow, unchanged.)
// --------------------------------------------------------------------------

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

EFI_STATUS
HardwareHandlerCheckLicense()
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
  EFI_STATUS                   Status;
  FAN_CONTROLLER_PROTOCOL      *mFanControllerProtocol;
  THERMAL_SENSOR_PROTOCOL      *mThermalSensorProtocol;
  LED_CONTROLLER_PROTOCOL      *mLedControllerProtocol;
  EFI_HANDLE                   Handle;
  
  Status = gSmst->SmmLocateProtocol(
              &gEfiHardwareLicenseProtocolGuid,
              NULL,
              (VOID **)&mHardwareManagerLicenseProtocol
              );
  if (EFI_ERROR(Status)) {
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
  int DanglingPointerCreated = 1;klee_assert(!DanglingPointerCreated);
  Status = gSmst->SmmInstallProtocolInterface(
                    &Handle,
                    &gEfiFanControllerProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    mFanControllerProtocol
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}
