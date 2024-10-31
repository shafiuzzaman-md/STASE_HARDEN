#ifndef _THERMAL_SENSOR_H_
#define _THERMAL_SENSOR_H_

#include <Library/SmmServicesTableLib.h>
#include <Library/IoLib.h>


#define TS_REG_ID             0x2e00
#define TS_REG_FUNC           0x2e04
#define TS_REG_VALUE          0x2e08

#define TS_GET_TEMPERATURE    1
#define TS_SET_THRESHOLD      2
#define TS_GET_SENSOR_STATUS  3
#define TS_RESET_SENSOR       4

extern EFI_GUID gEfiThermalSensorProtocolGuid;

EFI_STATUS
EFIAPI
GetTemperature (
  IN  UINTN  SensorId,
  OUT UINTN  *Temperature
);

EFI_STATUS
EFIAPI
SetThreshold (
  IN UINTN  SensorId,
  IN UINTN  Threshold
);

EFI_STATUS
EFIAPI
GetSensorStatus (
  IN  UINTN   SensorId,
  IN  UINT64  *Param1
);

EFI_STATUS
EFIAPI
ResetSensor (
  IN UINTN    SensorId,
  IN UINT64   Param1
);

typedef
EFI_STATUS
(EFIAPI *GET_TEMPERATURE)(
  IN  UINTN   SensorId,
  OUT UINTN   *Temperature
);

typedef
EFI_STATUS
(EFIAPI *SET_THRESHOLD)(
  IN UINTN  SensorId,
  IN UINTN  Threshold
);

typedef
EFI_STATUS
(EFIAPI *GET_SENSOR_STATUS)(
  IN UINTN    SensorId,
  IN UINT64   *Param1
);

typedef
EFI_STATUS
(EFIAPI *RESET_SENSOR)(
  IN UINTN    SensorId,
  IN UINT64   Param1
);

typedef struct _THERMAL_SENSOR_PROTOCOL {
  GET_TEMPERATURE   GetTemperature;
  SET_THRESHOLD     SetThreshold;
  GET_SENSOR_STATUS GetSensorStatus;
  RESET_SENSOR      ResetSensor;
} THERMAL_SENSOR_PROTOCOL;

#endif // _THERMAL_SENSOR_H_
