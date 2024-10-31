#ifndef _VOLTAGE_REGULATOR_H_
#define _VOLTAGE_REGULATOR_H_

#include <Library/SmmServicesTableLib.h>
#include <Library/IoLib.h>

#define VR_REG_ID           0x2f00
#define VR_REG_FUNC         0x2f04
#define VR_REG_VALUE        0x2f08
#define VR_REG_STATUS       VR_REG_VALUE

#define VR_SET_VOLTAGE      1
#define VR_GET_VOLTAGE      2
#define VR_MONITOR_POWER    3
#define VR_RESET_REGULATOR  4

extern EFI_GUID gEfiVoltageRegulatorProtocolGuid;

EFI_STATUS
EFIAPI
SetVoltage (
  IN UINTN RegulatorId,
  IN UINTN Millivolts
);

EFI_STATUS
EFIAPI
GetVoltage (
  IN UINTN  RegulatorId,
  OUT UINTN *Millivolts
);

EFI_STATUS
EFIAPI
MonitorPowerOutput (
  IN UINTN  RegulatorId,
  IN UINT64 Flags
);

EFI_STATUS
EFIAPI
ResetRegulator (
  IN UINTN  RegulatorId,
  IN UINT64 Flags
);

typedef
EFI_STATUS
(EFIAPI *SET_VOLTAGE)(
  IN UINTN RegulatorId,
  IN UINTN Millivolts
);

typedef
EFI_STATUS
(EFIAPI *GET_VOLTAGE)(
  IN UINTN  RegulatorId,
  OUT UINTN *Millivolts
);

typedef
EFI_STATUS
(EFIAPI *MONITOR_POWER_OUTPUT)(
  IN UINTN  RegulatorId,
  IN UINT64 Flags
);

typedef
EFI_STATUS
(EFIAPI *RESET_REGULATOR)(
  IN UINTN  RegulatorId,
  IN UINT64 Flags
);

typedef struct _VOLTAGE_REGULATOR_PROTOCOL {
  SET_VOLTAGE          SetVoltage;
  GET_VOLTAGE          GetVoltage;
  MONITOR_POWER_OUTPUT MonitorPowerOutput;
  RESET_REGULATOR      ResetRegulator;
} VOLTAGE_REGULATOR_PROTOCOL;

#endif // _VOLTAGE_REGULATOR_H_
