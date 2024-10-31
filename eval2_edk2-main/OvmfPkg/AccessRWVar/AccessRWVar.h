/** @file
  header file for sub call Read/Write Variable Operation.


**/

#ifndef _ACCESS_RWVAR_RW_VAR_H_
#define _ACCESS_RWVAR_RW_VAR_H_

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

#define MAX_LENGTH 1024

extern  EFI_GUID   gEfiRWVariableCommGuid;
extern  EFI_GUID   gEfiAccessRWVarProtocolGuid;

typedef struct _ACCESS_RWVAR_PROTOCOL ACCESS_RWVAR_PROTOCOL;

typedef struct {
  EFI_STATUS Status;
  UINT32     Signature;
  UINT32     AccessType;
  EFI_GUID   VariableGuid;
  UINT32     Attributes;
  UINT32     DataOffset;
  UINTN      DataLength;
  UINT8      Data[MAX_LENGTH]; 
} RW_VARIABLE_INFO;

typedef enum {
    NONE = 0,
    RESERVED0       = 1,
    RESERVED1       = 2,
    WRITE_DEF_ATTR  = 4,
    WRITE_ATTRS     = 8,
    READ_ATTRS      = 16,
    READ_DEF_ATTR   = 32,
    RESERVED2       = 64,
    RESERVED3       = 128   
} Access_Type;

typedef
EFI_STATUS
(EFIAPI *ACCESS_RWVAR_READ_FUNC_CB)(
  IN     CHAR16        *VariableName,
  IN     EFI_GUID      *VendorGuid,
  OUT    UINT32        *Attributes     OPTIONAL,
  IN OUT UINTN         *DataSize,
  OUT    VOID          *Data           OPTIONAL
);

typedef
EFI_STATUS
(EFIAPI *ACCESS_RWVAR_WRITE_FUNC_CB)(
  IN  CHAR16           *VariableName,
  IN  EFI_GUID         *VendorGuid,
  IN  UINT32           Attributes,
  IN  UINTN            DataSize,
  IN  VOID             *Data
);

struct _ACCESS_RWVAR_PROTOCOL {
  ACCESS_RWVAR_READ_FUNC_CB         Read;
  ACCESS_RWVAR_WRITE_FUNC_CB        Write;
};

EFI_STATUS
EFIAPI
RWVariableHandler (
  IN     EFI_HANDLE    DispatchHandle,
  IN     CONST VOID    *Context,
  IN OUT VOID          *CommBuffer,
  IN OUT UINTN         *CommBufferSize
);

EFI_STATUS
EFIAPI
AccessReadVariable (
  IN     CHAR16        *VariableName,
  IN     EFI_GUID      *VendorGuid,
  OUT    UINT32        *Attributes     OPTIONAL,
  IN OUT UINTN         *DataSize,
  OUT    VOID          *Data           OPTIONAL
);

EFI_STATUS
EFIAPI
AccessWriteVariable (
  IN  CHAR16           *VariableName,
  IN  EFI_GUID         *VendorGuid,
  IN  UINT32           Attributes,
  IN  UINTN            DataSize,
  IN  VOID             *Data
);

#endif
