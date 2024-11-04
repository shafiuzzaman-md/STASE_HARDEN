#ifndef _SMM_LEGACY_H_
#define _SMM_LEGACY_H_

#include "../../eval2_edk2-main/MdeModulePkg/Include/Protocol/SmmLegacyDispatch.h"
#include "../../eval2_edk2-main/MdeModulePkg/Include/Protocol/SmmVariable.h"
#include "../../eval2_edk2-main/MdePkg/Include/Library/SmmServicesTableLib.h"
#include "../../eval2_edk2-main/MdePkg/Include/Library/MmServicesTableLib.h"
//#include "PiSmmCpuDxeSmm.h"

// AccessRWVar ACCESS_RWVAR_PROTOCOL 
#include "../../eval2_edk2-main/OvmfPkg/AccessRWVar/AccessRWVar.h"

#define SMM_LEGACY_HANDLER_SIGNATURE  SIGNATURE_32('s','l','h','s')
#define MAXIMUM_SWI_VALUE         0xFF
#define SMM_CONTROL_PORT          0xB2
#define SMM_DATA_PORT             0xB3

extern EFI_GUID gEfiSmmLegacyVariableGuid;
extern EFI_GUID gEfiSmmVariableProtocolGuid;

VOID        SmmLegacyExitPoint ( VOID );
EFI_STATUS  GetNvConfiguration ( VOID );

typedef struct {
  UINTN         Signature;
  LIST_ENTRY    Link;
  EFI_HANDLE    DispatchHandle;
  UINTN         InputValue;
  UINTN         DispatchFunction;
} EFI_SMM_LEGACY_DISPATCH_CONTEXT;

typedef enum {
  SMM_LEGACY_ENABLED              = 0x01,
  SMM_LEGACY_MULTIPLE_DISPATCH    = 0x02,
  SMM_LEGACY_SOMETHING_WEIRD      = 0x04,

} SMM_LEGACY_CONFIGURATION;


EFI_STATUS
EFIAPI
SmmLegacyEntryPoint (
  VOID
  );
#endif