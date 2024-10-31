#ifndef _FAN_CONTROLLER_H_
#define _FAN_CONTROLLER_H_

#include <Library/SmmServicesTableLib.h>
#include <Library/IoLib.h>

#define FC_REG_ID       0x2d00
#define FC_REG_FUNC     0x2d04
#define FC_REG_VALUE    0x2d08
#define FC_REG_STATUS   FC_REG_VALUE

#define FC_SET_SPEED    1
#define FC_GET_SPEED    2
#define FC_SET_MODE     3
#define FC_GET_STATUS   4

extern EFI_GUID gEfiFanControllerProtocolGuid;


EFI_STATUS
EFIAPI
SetFanSpeed(
  IN UINTN FanId,
  IN UINTN Speed
);

EFI_STATUS
EFIAPI
GetFanSpeed(
  IN UINTN  FanId,
  OUT UINTN *Speed
);

EFI_STATUS
EFIAPI
SetFanMode(
  IN UINTN  FanId,
  IN UINT64 Mode
);

EFI_STATUS
EFIAPI
GetFanStatus(
  IN UINTN  FanId,
  IN UINT64 Flags,
  OUT UINTN *Value
);

typedef
EFI_STATUS
(EFIAPI *SET_FAN_SPEED)(
  IN UINTN FanId,
  IN UINTN Speed
);

typedef
EFI_STATUS
(EFIAPI *GET_FAN_SPEED)(
  IN  UINTN   FanId,
  OUT UINTN   *Speed
);

typedef
EFI_STATUS
(EFIAPI *SET_FAN_MODE)(
  IN UINTN  FanId,
  IN UINT64 Mode
);

typedef
EFI_STATUS
(EFIAPI *GET_FAN_STATUS)(
  IN  UINTN   FanId,
  IN  UINT64  Flags,
  OUT UINTN   *Value
);

typedef struct _FAN_CONTROLLER_PROTOCOL {
  SET_FAN_SPEED    SetFanSpeed;
  GET_FAN_SPEED    GetFanSpeed;
  SET_FAN_MODE     SetFanMode;
  GET_FAN_STATUS   GetFanStatus;
} FAN_CONTROLLER_PROTOCOL;

#endif // _FAN_CONTROLLER_H_