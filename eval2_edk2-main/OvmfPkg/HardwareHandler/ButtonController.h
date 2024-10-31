#ifndef _BUTTON_CONTROLLER_H_
#define _BUTTON_CONTROLLER_H_

#include <Library/SmmServicesTableLib.h>
#include <Library/IoLib.h>

extern EFI_GUID gEfiButtonControllerProtocolGuid;

#define BC_REG_ID           0x2a00  // Receives ButtonId
#define BC_REG_FUNC         0x2a04  // Receives function code
#define BC_REG_VALUE        0x2a08  // Receives second value or Flags, or read as BC_REG_STATUS for status

#define BC_FUNC_GET_STATE   0x1
#define BC_FUNC_SET_LOCK    0x2
#define BC_FUNC_TRIGGER     0x3
#define BC_FUNC_RESET       0x4

#define BC_STATUS_SUCCESS   0x80000000
#define BC_STATUS_RESULT    0x7FFFFFFF  // Lower 31 bits contain the result if any


typedef
EFI_STATUS
(EFIAPI *GET_BUTTON_STATE)(
  IN  UINTN  ButtonId,
  OUT UINT64 *State
);

typedef
EFI_STATUS
(EFIAPI *SET_BUTTON_LOCK)(
  IN UINTN  ButtonId,
  IN UINT64 LockState
);

typedef
EFI_STATUS
(EFIAPI *TRIGGER_BUTTON_ACTION)(
  IN UINTN  ButtonId,
  IN UINT64 Flags
);

typedef
EFI_STATUS
(EFIAPI *RESET_BUTTON_STATE)(
  IN UINTN  ButtonId,
  IN UINT64 Flags
);

EFI_STATUS
EFIAPI
GetButtonState (
  IN UINTN    ButtonId,
  OUT UINT64  *State
);

EFI_STATUS
EFIAPI
SetButtonLock (
  IN UINTN  ButtonId,
  IN UINT64 LockState
);

EFI_STATUS
EFIAPI
TriggerButtonAction (
  IN UINTN  ButtonId,
  IN UINT64 Flags
);

EFI_STATUS
EFIAPI
ResetButtonState (
  IN UINTN  ButtonId,
  IN UINT64 Flags
);

typedef struct _BUTTON_CONTROLLER_PROTOCOL {
  GET_BUTTON_STATE      GetButtonState;
  SET_BUTTON_LOCK       SetButtonLock;
  TRIGGER_BUTTON_ACTION TriggerButtonAction;
  RESET_BUTTON_STATE    ResetButtonState;
} BUTTON_CONTROLLER_PROTOCOL;

#endif // _BUTTON_CONTROLLER_H_
