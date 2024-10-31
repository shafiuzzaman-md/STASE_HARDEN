/** @file
  header file for Executing some code on the Smm Stack
**/

#ifndef _KEYVAULT_H_
#define _KEYVAULT_H_

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

#include "KeyHash.h"

#define VAULT_PAGE_COUNT  2
#define VAULT_KEY_COUNT   64
#define VAULT_KEY_SIZE    64
#define VAULT_HASH_SIZE   32

extern  EFI_GUID   gEfiKeyVaultCommGuid;
extern  EFI_GUID   gEfiKeyVaultProtocolGuid;

typedef CHAR8      TOKEN_NAME[16];
typedef CHAR8      HASH_CONTENT[32];
typedef CHAR8      TOKEN_CONTENT[64]; 
typedef CHAR8      TOKEN_SECRET[64];

typedef enum {
  VAULT_KEY_CREATE,
  VAULT_KEY_READ
} VAULT_KEY_ACTION;

typedef struct {
  EFI_STATUS        Status;
  VAULT_KEY_ACTION  Action;
  EFI_GUID          VendorGuid;
  TOKEN_NAME        TokenName;
  TOKEN_CONTENT     TokenContent;
  TOKEN_SECRET      TokenSecret;
} KEY_VAULT_COMM;

typedef EFI_STATUS
(EFIAPI *KEY_VAULT_CREATE_KEY)(
  KEY_VAULT_COMM*
);

typedef EFI_STATUS
(EFIAPI *KEY_VAULT_READ_KEY)(
  KEY_VAULT_COMM*,
  KEY_VAULT_COMM*
);

typedef struct {
  KEY_VAULT_CREATE_KEY      Create;
  KEY_VAULT_READ_KEY        Read;
} KEY_VAULT_PROTOCOL;

typedef struct {
  EFI_PHYSICAL_ADDRESS    BaseAddress;
  TOKEN_CONTENT           *TokenList;
  TOKEN_NAME              *NameList;
  HASH_CONTENT            *HashList;
  EFI_GUID                *GuidList;
  UINTN                   MappedLength;
  UINTN                   UsedKeysMap;
} KEY_VAULT_INFO;

#define FIRST_UNSET_BIT(num) (__builtin_ctzll(~(num)))

#endif
