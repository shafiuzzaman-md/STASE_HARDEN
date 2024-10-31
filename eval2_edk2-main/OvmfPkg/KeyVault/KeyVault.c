/** @file
   Executing some code on the Smm Stack Driver.

**/
#include "KeyVault.h"

KEY_VAULT_INFO      VaultInfo;
KEY_VAULT_PROTOCOL  VaultProto;
EFI_HANDLE          DispatchHandle;
EFI_HANDLE          ProtocolHandle;

BOOLEAN
CheckKeyMatches(
  UINTN           KeyIndex,
  CHAR8           *Hashed,
  CHAR8           *TokenName,
  UINTN           NameLength,
  EFI_GUID        *VendorGuid
  )
{
  if (CompareMem(VendorGuid, &VaultInfo.GuidList[KeyIndex], sizeof(EFI_GUID)) != 0) {
    return FALSE;
  }

  if (CompareMem(TokenName, VaultInfo.NameList[KeyIndex], NameLength) != 0) {
    return FALSE;
  }

  if (CompareMem(Hashed, VaultInfo.HashList[KeyIndex], sizeof(HASH_CONTENT)) != 0) { 
    return FALSE;
  }

  return TRUE;
}

EFI_STATUS
EFIAPI
CreateVaultKey(
  IN OUT KEY_VAULT_COMM    *Buffer
  )
{
  UINTN         KeyIndex;
  UINTN         SecretLength;
  UINTN         NameLength;
  HASH_CONTENT  Hashed;

  if ((VaultInfo.UsedKeysMap + 1) == 0) {
    DEBUG((DEBUG_ERROR, "%a: Cannot create new key - insufficient space\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  if ((SecretLength = AsciiStrLen(Buffer->TokenSecret)) > sizeof(TOKEN_SECRET)) {
    DEBUG((DEBUG_ERROR, "%a: Invalid secret key length\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if ((NameLength = AsciiStrLen(Buffer->TokenName)) > sizeof(TOKEN_NAME)) {
    DEBUG((DEBUG_ERROR, "%a: Invalid token name length\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  KeyIndex = __builtin_ctzll(~VaultInfo.UsedKeysMap);
  VaultInfo.UsedKeysMap = VaultInfo.UsedKeysMap | (1 << KeyIndex);

  CalculateHash(Buffer->TokenSecret, SecretLength, (UINT32 *)Hashed);
  CopyMem(VaultInfo.HashList[KeyIndex], Hashed, sizeof(HASH_CONTENT));

  SetMem(Buffer->TokenSecret, sizeof(TOKEN_SECRET), 0x00);
  SetMem(Hashed, sizeof(HASH_CONTENT), 0x00);

  CopyMem(VaultInfo.TokenList[KeyIndex], Buffer->TokenContent, sizeof(TOKEN_CONTENT));
  SetMem(Buffer->TokenContent, sizeof(TOKEN_CONTENT), 0x00);

  CopyMem(&VaultInfo.GuidList[KeyIndex], &Buffer->VendorGuid, sizeof(EFI_GUID));
  CopyMem(VaultInfo.NameList[KeyIndex], Buffer->TokenName, NameLength);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
ReadVaultKey(
  IN      KEY_VAULT_COMM    *Buffer,
  IN OUT  KEY_VAULT_COMM    *ReturnBuffer
  )
{
  UINTN                   KeyMask;
  UINTN                   KeyIndex;
  UINTN                   SecretLength;
  UINTN                   NameLength;
  HASH_CONTENT            Hashed;

  if ((SecretLength = AsciiStrLen(Buffer->TokenSecret)) > sizeof(TOKEN_SECRET)) {
    DEBUG((DEBUG_ERROR, "%a: Invalid secret key length\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if ((NameLength = AsciiStrLen(Buffer->TokenName)) > sizeof(TOKEN_NAME)) {
    DEBUG((DEBUG_ERROR, "%a: Invalid token name length\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  CalculateHash(Buffer->TokenSecret, SecretLength, (UINT32 *)Hashed);
  SetMem(Buffer->TokenSecret, sizeof(TOKEN_SECRET), 0x00);

  for (KeyMask = 1; KeyMask != 0; KeyMask = KeyMask << 1) {
    if ((VaultInfo.UsedKeysMap & KeyMask) != 0) {
      KeyIndex = __builtin_ctzll(KeyMask);
      if (CheckKeyMatches(KeyIndex, Hashed, Buffer->TokenName, NameLength, &Buffer->VendorGuid)) {
        CopyMem(ReturnBuffer->TokenContent, VaultInfo.TokenList[KeyIndex], sizeof(TOKEN_CONTENT));        
        SetMem(Hashed, sizeof(HASH_CONTENT), 0x00);
        return EFI_SUCCESS;
      }
    }
  }

  SetMem(Hashed, sizeof(HASH_CONTENT), 0x00);
  return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
KeyVaultHandler (
  IN     EFI_HANDLE    DispatchHandle,
  IN     CONST VOID    *Context,
  IN OUT VOID          *CommBuffer,
  IN OUT UINTN         *CommBufferSize
  )
{
  EFI_STATUS            Status;
  KEY_VAULT_COMM        *Buffer;
  KEY_VAULT_COMM        *ReturnBuffer;
  UINTN                 BufferSize;

  if (CommBuffer == NULL || CommBufferSize == NULL) {
    DEBUG((DEBUG_INFO, "%a: Invalid CommBuffer or CommBufferSize\n", __func__));
    ((KEY_VAULT_COMM *)CommBuffer)->Status = EFI_INVALID_PARAMETER;    
    return EFI_SUCCESS;
  }

  BufferSize    = *CommBufferSize - 0x18;
  ReturnBuffer  = CommBuffer;

  if (BufferSize > sizeof(KEY_VAULT_COMM)) {
    DEBUG((DEBUG_INFO, "%a: Buffer size exceeds structure boundaries (%ld > %d) !!!\n", __func__, BufferSize, sizeof(KEY_VAULT_COMM)));
    ((KEY_VAULT_COMM *)CommBuffer)->Status = EFI_INVALID_PARAMETER;
    return EFI_SUCCESS;
  }

  Status = gSmst->SmmAllocatePool (
            EfiRuntimeServicesData,
            BufferSize,
            (VOID **)&Buffer
            );
   
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a: Failed to allocate buffer\n", __func__));
    ((KEY_VAULT_COMM *)CommBuffer)->Status = EFI_OUT_OF_RESOURCES;
    return EFI_SUCCESS;
  }

  CopyMem(Buffer, CommBuffer, sizeof(KEY_VAULT_COMM));
  SetMem(ReturnBuffer->TokenSecret, sizeof(TOKEN_SECRET), 0x00);
  SetMem(ReturnBuffer->TokenContent, sizeof(TOKEN_CONTENT), 0x00);

  switch(Buffer->Action) {
  case VAULT_KEY_CREATE:
    Status = CreateVaultKey(Buffer);
    break;
  case VAULT_KEY_READ:
    Status = ReadVaultKey(Buffer, ReturnBuffer);
    break;
  default:
    DEBUG((DEBUG_INFO, "%a: Invalid Vault Key action\n"));
    Status = EFI_INVALID_PARAMETER;
  }

  SetMem(Buffer, sizeof(KEY_VAULT_COMM), 0x00);

  ReturnBuffer->Status = Status;

  return EFI_SUCCESS;
}

EFI_STATUS
VaultKeyAbort(
  VOID
  )
{
  if (VaultInfo.BaseAddress != (EFI_PHYSICAL_ADDRESS)NULL) {
    gSmst->SmmFreePages(VaultInfo.BaseAddress, VAULT_PAGE_COUNT);
    VaultInfo.BaseAddress   = (EFI_PHYSICAL_ADDRESS)NULL;
    VaultInfo.TokenList     = (TOKEN_CONTENT *)NULL;
    VaultInfo.HashList      = (HASH_CONTENT *)NULL;
    VaultInfo.GuidList      = (EFI_GUID *)NULL;
    VaultInfo.NameList      = (TOKEN_NAME *)NULL;
    VaultInfo.UsedKeysMap   = (UINTN)NULL;
  }

  if (DispatchHandle != (EFI_HANDLE)NULL) {
    gMmst->MmiHandlerUnRegister(&DispatchHandle);
    DispatchHandle = (EFI_HANDLE)NULL;
  }

  VaultProto.Read     = NULL;
  VaultProto.Create   = NULL;

  if (ProtocolHandle != (EFI_HANDLE)NULL) {
    gMmst->MmUninstallProtocolInterface(
            &ProtocolHandle,
            &gEfiKeyVaultProtocolGuid,
            &VaultProto
            );
  }

  return EFI_ABORTED;
}

EFI_STATUS
EFIAPI
KeyVaultInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;

  VaultProto.Create       = NULL;
  VaultProto.Read         = NULL;
  VaultInfo.BaseAddress   = 0x00;
  DispatchHandle          = NULL;

  if (sizeof(EFI_PHYSICAL_ADDRESS) <= 4) {
    return EFI_ABORTED;
  }
  
  Status = gSmst->SmmAllocatePages(
            AllocateAnyPages,
            EfiRuntimeServicesData,
            VAULT_PAGE_COUNT,
            &VaultInfo.BaseAddress
            );

  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a: could not allocate vault pages\n", __func__));
    return EFI_ABORTED;
  }

  VaultInfo.TokenList     = (TOKEN_CONTENT *)VaultInfo.BaseAddress;
  VaultInfo.HashList      = (HASH_CONTENT *)(VaultInfo.BaseAddress + (VAULT_KEY_COUNT * sizeof(TOKEN_CONTENT)));
  VaultInfo.GuidList      = (EFI_GUID *)((EFI_PHYSICAL_ADDRESS)VaultInfo.HashList + (VAULT_KEY_COUNT * sizeof(HASH_CONTENT)));
  VaultInfo.NameList      = (TOKEN_NAME *)((EFI_PHYSICAL_ADDRESS)VaultInfo.GuidList + (VAULT_KEY_COUNT * sizeof(EFI_GUID)));
  VaultInfo.MappedLength  = (VAULT_PAGE_COUNT * EFI_PAGE_SIZE);
  VaultInfo.UsedKeysMap   = (UINTN)0x00;

  Status = gMmst->MmiHandlerRegister(
            KeyVaultHandler,
            &gEfiKeyVaultCommGuid,
            &DispatchHandle
            );

  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a: could not register mmi handler\n", __func__));
    return VaultKeyAbort();
  }

  VaultProto.Create   = CreateVaultKey;
  VaultProto.Read     = ReadVaultKey;

  Status = gMmst->MmInstallProtocolInterface(
            &ProtocolHandle,
            &gEfiKeyVaultProtocolGuid,
            EFI_NATIVE_INTERFACE,
            &VaultProto
            );

  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a: could not register protocol\n", __func__));
    return VaultKeyAbort();
  }

  return Status;
}
