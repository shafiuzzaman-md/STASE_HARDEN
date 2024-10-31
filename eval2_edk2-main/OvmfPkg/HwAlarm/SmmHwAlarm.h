/** @file
  Master header file for Hardware HwAlarm Control.


**/
#ifndef _SMM_HWALARM_H_
#define _SMM_HWALARM_H_

#include <Library/PcdLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/SmmCommunication.h>
#include <Guid/EventGroup.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/MmServicesTableLib.h>
#include <Protocol/MmReportStatusCodeHandler.h>

// #include <Protocol/Led.h>

#define REGISTER_RSC           0x1
#define RE_REGISTER_RSC        0x2
#define UNREGISTER_FROM_RSC    0x3

typedef enum {
  HWALARM_OFF = 0x0,
  KLAXTON = 0x10,
  POWERPLANT_CASUALTY = 0x20,
  GENERAL_ALARM = 0x30,
  DIVING_BELL = 0x40,
  MAX_HWALARM_TYPE
} HWALARM_STATUS_TYPES;


typedef struct {
  UINTN         Function;
  EFI_STATUS    ReturnStatus;
  BOOLEAN       Disable;             
} SMM_HWALARM_FUNCTION_COMMUNICATION;

//
// Size of SMM communicate header, without including the payload.
//
#define SMM_COMMUNICATE_HEADER_SIZE  (OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data))

//
// Size of Smm Function Communication struct.
//
#define SMM_HWALARM_FUNCTION_COMMUNICATION_SIZE  (sizeof(SMM_HWALARM_FUNCTION_COMMUNICATION))

typedef struct _EFI_MM_REAR_SPEAKER_PROTOCOL EFI_MM_REAR_SPEAKER_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *EFI_SOUND_ALARM_FUNCTION)(
  IN EFI_MM_REAR_SPEAKER_PROTOCOL    *This
  );

struct _EFI_MM_REAR_SPEAKER_PROTOCOL {
  UINTN Index;
  UINTN Function;
  UINTN Duration;
  EFI_SOUND_ALARM_FUNCTION StatusUpdate;
};

#endif