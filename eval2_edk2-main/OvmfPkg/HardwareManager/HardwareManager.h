#ifndef _HARDWARE_MANAGER_H_
#define _HARDWARE_MANAGER_H_

#include <PiSmm.h>
#include <Uefi.h>
#include <Protocol/SmmBase2.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#include "../HardwareHandler/ThermalSensor.h"
#include "../HardwareHandler/FanController.h"
#include "../HardwareHandler/VoltageRegulator.h"
#include "../HardwareHandler/LEDController.h"
#include "../HardwareHandler/ButtonController.h"
#include "../ProcessFirmware/ProcessFirmware.h"

extern EFI_GUID gEfiHardwareHandlerReadyProtocolGuid;
extern EFI_GUID gEfiProcessFirmwareReadyProtocolGuid;

extern EFI_GUID gEfiHardwareManagerCommGuid;
extern EFI_GUID gEfiHardwarHandlerVariableGuid;
extern EFI_GUID gEfiSmmVariableProtocolGuid;
extern EFI_GUID gEfiHardwareLicenseProtocolGuid;

#define HARDWARE_HANDLER_LICENSE_VARIABLE L"HardwareHandlerLicense"

#define HW_FUNC_HH_SET_LICENSE_KEY         0X00

#define HW_FUNC_LC_SET_LED_COLOR           0x01
#define HW_FUNC_LC_SET_LED_BRIGHTNESS      0x02
#define HW_FUNC_LC_SET_LED_EFFECT          0x03
#define HW_FUNC_LC_RESET_LED               0x04

#define HW_FUNC_VR_SET_VOLTAGE             0x11
#define HW_FUNC_VR_GET_VOLTAGE             0x12
#define HW_FUNC_VR_MONITOR_POWER_OUTPUT    0x13
#define HW_FUNC_VR_RESET_REGULATOR         0x14

#define HW_FUNC_BC_GET_BUTTON_STATE        0x21
#define HW_FUNC_BC_SET_BUTTON_LOCK         0x22
#define HW_FUNC_BC_TRIGGER_BUTTON_ACTION   0x23
#define HW_FUNC_BC_RESET_BUTTON_STATE      0x24

#define HW_FUNC_TS_GET_TEMPERATURE         0x31
#define HW_FUNC_TS_SET_THRESHOLD           0x32
#define HW_FUNC_TS_GET_SENSOR_STATUS       0x33
#define HW_FUNC_TS_RESET_SENSOR            0x34

#define HW_FUNC_FC_SET_FAN_SPEED           0x41
#define HW_FUNC_FC_GET_FAN_SPEED           0x42
#define HW_FUNC_FC_SET_FAN_MODE            0x43
#define HW_FUNC_FC_GET_FAN_STATUS          0x44

#define HW_FUNC_PF_BC_CHECK_FIRMWARE       0x51
#define HW_FUNC_PF_BC_LOAD_FIRMWARE        0x52
#define HW_FUNC_PF_FC_CHECK_FIRMWARE       0x53
#define HW_FUNC_PF_FC_LOAD_FIRMWARE        0x54
#define HW_FUNC_PF_LC_CHECK_FIRMWARE       0x55
#define HW_FUNC_PF_LC_LOAD_FIRMWARE        0x56
#define HW_FUNC_PF_TS_CHECK_FIRMWARE       0x57
#define HW_FUNC_PF_TS_LOAD_FIRMWARE        0x58
#define HW_FUNC_PF_VR_CHECK_FIRMWARE       0x59
#define HW_FUNC_PF_VR_LOAD_FIRMWARE        0x5a

#define FIRMWARE_CHUNK_LENGTH              101
#define FIRMWARE_CONTENT_LENGTH            704
#define LICENSE_KEY_LENGTH                 64

typedef UINT64 FIRMWARE_CHUNK[FIRMWARE_CHUNK_LENGTH];
typedef UINT8  FIRMWARE_CONTENT[FIRMWARE_CONTENT_LENGTH];
typedef UINT8  LICENSE_KEY[LICENSE_KEY_LENGTH];

#define FIRMWARE_CHUNK_SIZE   sizeof(FIRMWARE_CHUNK)
#define LICENSE_MAGIC         0xdeadf00d

typedef struct {
  EFI_GUID            VendorGuid;
  EFI_GUID            HardwareGuid;
  VOID                *FirmwareHead;
  VOID                *FirmwareBody;
  LICENSE_KEY         LicenseKey;
  FIRMWARE_CONTENT    Content;
  UINT64              Tail;
} FIRMWARE_CHUNK_METADATA;
#define METADATA_SIZE OFFSET_OF(FIRMWARE_CHUNK_METADATA, Content)

EFI_STATUS HardwareManagerSetLicense(IN VOID *Data);

typedef struct _HARDWARE_MANAGER_COMM_BUFFER {
    UINT64 FunctionCode;
    UINT64 Param1;
    UINT64 Param2;
    UINT64 Param3;
    UINT64 Param4;
    UINT64 Result;
    EFI_STATUS Status;
    FIRMWARE_CHUNK FirmwareChunk;
} HARDWARE_MANAGER_COMM_BUFFER;

typedef
EFI_STATUS
(EFIAPI *HARDWARE_MANAGER_GET_LICENSE)(
    OUT UINT8 *License
);

typedef
EFI_STATUS
(EFIAPI *HARDWARE_MANAGER_VALIDATE_LICENSE)(
    IN UINT8   *License
);

typedef struct _HARDWARE_MANAGER_LICENSE_PROTOCOL {
  HARDWARE_MANAGER_GET_LICENSE      GetLicense;
  HARDWARE_MANAGER_VALIDATE_LICENSE ValidateLicense;
} HARDWARE_MANAGER_LICENSE_PROTOCOL;

typedef struct _NOTIFY_PROTOCOL_READY_PROTOCOL {
  UINT32      ReadyFlags;
} NOTIFY_PROTOCOL_READY_PROTOCOL;

#endif // _HARDWARE_MANAGER_H_
