/** @file
  Master header file for Query Variable Info.

**/

#ifndef _QUERY_VARIABLE_INFO_H_
#define _QUERY_VARIABLE_INFO_H_

#include <PiSmm.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>


typedef struct {
  UINT32   Attributes;
  UINTN    MaximumVariableStorageSize;
  UINTN    RemainingVariableStorageSize;
  UINTN    MaximumVariableSize;
} QUERY_VARIABLE_INFO;

#endif
