#ifndef __PROCESS_FIRMWARE_H__
#define __PROCESS_FIRMWARE_H__

#include <Library/MmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/SmmMemLib.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/SmmMemoryAttribute.h>
#include <Protocol/SmmLegacyDispatch.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/IoLib.h>

extern EFI_GUID gEfiProcessFirmwareProtocolGuid;
extern EFI_GUID gEfiProcessFirmwareReadyProtocolGuid;

#define BC_FW_MAGIC   0xd1ffd1ffd1ffffd1
#define FC_FW_MAGIC   0xd1d1ffd1ffd1ffff
#define LC_FW_MAGIC   0xffd1d1ffd1ffd1ff
#define TS_FW_MAGIC   0xffffd1d1ffd1ffd1
#define VR_FW_MAGIC   0xd1ffffd1d1ffd1ff

#define TS_VALIDATE_CHECKSUM(input) \
  (((input) ^ 0xA7C8D9B234F1E6A5) ^                          \
  (((input) * 0x92384723847384) ^ ((input) >> 3)) ^          \
  (((input) << 7) ^ (0x7B9C2D3F4E6A5B8A)) ^                  \
  (((input) >> 5) * 0x38474A7D9F4C3B2A) ^                    \
  (((input) << 11) + 0x48374AC8D9B7E6A5) ^                   \
  (((input) * 0x93847B8D7F6E5C3A) + ((input) >> 2)) ^        \
  (((input) << 9) - 0x28374AC7D9F8B7A6) ^                    \
  (((input) * 0x8472A5B8D7E9C6F4) + ((input) << 3)) ^        \
  (((input) >> 4) + 0x7384ACD8B9F6E7C5) ^                    \
  (((input) * 0x4738A7F9D8C7B6A4) - ((input) >> 6)) ^        \
  (((input) << 8) ^ 0x59384AC7D9F8B6C4) ^                    \
  (((input) >> 7) * 0x12947A8F6E5D3C2B) ^                    \
  (((input) * 0xA5C9D7F6E8B7C3D2) + ((input) << 10)) ^       \
  (((input) >> 9) + 0xA3B7F9E6D8C5B4A9) ^                    \
  (((input) * 0x12847A8C7D9F6B5A) - ((input) >> 11)) ^       \
  (((input) << 12) ^ 0x93847AC7D9F5B4E6) ^                   \
  (((input) >> 13) * 0x38472A9F7D6E5B4C) ^                   \
  (((input) << 14) - 0x4837A9C8D6B5E7F9))

#define BC_REG_ID         0x2a00
#define BC_REG_FUNC       0x2a04
#define BC_REG_VALUE      0x2a08
#define BC_REG_STATUS     BC_REG_VALUE

#define FC_REG_ID         0x2d00
#define FC_REG_FUNC       0x2d04
#define FC_REG_VALUE      0x2d08
#define FC_REG_STATUS     FC_REG_VALUE

#define LC_REG_ID         0x2b00
#define LC_REG_FUNC       0x2b04
#define LC_REG_VALUE      0x2b08
#define LC_REG_STATUS     LC_REG_VALUE

#define TS_REG_ID         0x2e00
#define TS_REG_FUNC       0x2e04
#define TS_REG_VALUE      0x2e08
#define TS_REG_STATUS     TS_REG_VALUE

#define VR_REG_ID         0x2f00
#define VR_REG_FUNC       0x2f04
#define VR_REG_VALUE      0x2f08
#define VR_REG_STATUS     VR_REG_VALUE

#define BC_FUNC_LOAD    0xff
#define FC_FUNC_LOAD    0xff
#define LC_FUNC_LOAD    0xff
#define TS_FUNC_LOAD    0xff
#define VR_FUNC_LOAD    0xff

EFI_STATUS
EFIAPI
BCCheckFirmware (
  UINT64 *Settings
);

EFI_STATUS
EFIAPI
BCLoadFirmware (
  UINT64 Id,
  UINT64 *Settings
);

EFI_STATUS
EFIAPI
FCCheckFirmware (
  UINT64 *Settings
);

EFI_STATUS
EFIAPI
FCLoadFirmware (
  UINT64 Id,
  UINT64 *Settings
);

EFI_STATUS
EFIAPI
LCCheckFirmware (
  UINT64 *Settings
);

EFI_STATUS
EFIAPI
LCLoadFirmware (
  UINT64 Id,
  UINT64 *Settings
);

EFI_STATUS
EFIAPI
TSCheckFirmware (
  UINT64 *Settings
);

EFI_STATUS
EFIAPI
TSLoadFirmware (
  UINT64 Id,
  UINT64 *Settings
);

EFI_STATUS
EFIAPI
VRCheckFirmware (
  UINT64 *Settings
);

EFI_STATUS
EFIAPI
VRLoadFirmware (
  UINT64 Id,
  UINT64 *Settings
);

typedef struct _PROCESS_FIRMWARE_PROTOCOL {
    EFI_STATUS (EFIAPI *BCCheckFirmware)(UINT64 *Settings);
    EFI_STATUS (EFIAPI *BCLoadFirmware)(UINT64 Id, UINT64 *Settings);

    EFI_STATUS (EFIAPI *FCCheckFirmware)(UINT64 *Settings);
    EFI_STATUS (EFIAPI *FCLoadFirmware)(UINT64 Id, UINT64 *Settings);

    EFI_STATUS (EFIAPI *LCCheckFirmware)(UINT64 *Settings);
    EFI_STATUS (EFIAPI *LCLoadFirmware)(UINT64 Id, UINT64 *Settings);

    EFI_STATUS (EFIAPI *TSCheckFirmware)(UINT64 *Settings);
    EFI_STATUS (EFIAPI *TSLoadFirmware)(UINT64 Id, UINT64 *Settings);

    EFI_STATUS (EFIAPI *VRCheckFirmware)(UINT64 *Settings);
    EFI_STATUS (EFIAPI *VRLoadFirmware)(UINT64 Id, UINT64 *Settings);
} PROCESS_FIRMWARE_PROTOCOL;

#endif
