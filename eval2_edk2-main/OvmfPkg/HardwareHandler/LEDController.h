#ifndef _LED_CONTROLLER_H_
#define _LED_CONTROLLER_H_

#include <Library/SmmServicesTableLib.h>
#include <Library/IoLib.h>


extern EFI_GUID gEfiLedControllerProtocolGuid;

#define LC_REG_ID               0x2b00
#define LC_REG_FUNC             0x2b04
#define LC_REG_VALUE            0x2b08
#define LC_REG_STATUS           LC_REG_VALUE

#define LC_COLOR                1
#define LC_BRIGHTNESS           2
#define LC_EFFECT               4

#define LC_RESET                0xFF


EFI_STATUS
EFIAPI
SetLedColor(
  IN UINTN  LedId,
  IN UINT64 RGB
);

EFI_STATUS
EFIAPI
SetLedBrightness(
  IN UINTN  LedId,
  IN UINT64 Brightness,
  IN UINT64 Flags
);

EFI_STATUS
EFIAPI
SetLedEffect(
  IN UINTN  LedId,
  IN UINT64 EffectType,
  IN UINT64 Speed
);

EFI_STATUS
EFIAPI
ResetLed(
  IN UINTN  LedId,
  IN UINT64 Flags
);

typedef
EFI_STATUS
(EFIAPI *SET_LED_COLOR)(
  IN UINTN  LedId,
  IN UINT64 RGB
);

typedef
EFI_STATUS
(EFIAPI *SET_LED_BRIGHTNESS)(
  IN UINTN  LedId,
  IN UINT64 Brightness,
  IN UINT64 Flags
);

typedef
EFI_STATUS
(EFIAPI *SET_LED_EFFECT)(
  IN UINTN  LedId,
  IN UINT64 EffectType,
  IN UINT64 Speed
);

typedef
EFI_STATUS
(EFIAPI *RESET_LED)(
  IN UINTN  LedId,
  IN UINT64 Flags
);

typedef struct _LED_CONTROLLER_PROTOCOL {
  SET_LED_COLOR       SetLedColor;
  SET_LED_BRIGHTNESS  SetLedBrightness;
  SET_LED_EFFECT      SetLedEffect;
  RESET_LED           ResetLed;
}  LED_CONTROLLER_PROTOCOL;

#endif // _LED_CONTROLLER_H_
