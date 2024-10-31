/** @file
  header file for Executing some code on the Smm Stack
**/

#ifndef _RDTSCONTROL_H_
#define _RDTSCONTROL_H_

#include <PiSmm.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/SmmMemLib.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/SmmMemoryAttribute.h>
#include <Protocol/SmmLegacyDispatch.h>

#define MAX_LENGTH 1024

typedef enum RDTSC_CONTROL_TYPE RDTSC_CONTROL_TYPE;

enum RDTSC_CONTROL_TYPE {
  TYPE_SET_RDTSC,
  TYPE_GET_RDTSC,
  TYPE_CLR_RDTSC
};

typedef struct {
  CHAR8     SetRdtsc[12];
  CHAR8     ClrRdtsc[12];
  CHAR8     GetRdtsc[12];
} RDTSC_CONTROL_OPS;

typedef struct {
  EFI_STATUS          Status;
  UINTN               Result;
  RDTSC_CONTROL_TYPE  Type;
  CHAR8               Data[1];
} RDTSC_CONTROL_DATA;

#endif
