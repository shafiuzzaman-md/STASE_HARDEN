/** @file
  Access Read/Write Variable Driver.

**/

#include "../../eval2_edk2-main//OvmfPkg/AccessRWVar/AccessRWVar.h"



EFI_SMM_VARIABLE_PROTOCOL  *mSmmVariable    = NULL;
ACCESS_RWVAR_PROTOCOL      gAccessRWVar     = { NULL, NULL };

/**
  This function is the read/write variable smi handler.

  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     Context         Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in,out] CommBuffer      A pointer to a collection of data in memory that will be conveyed
                                 from a non-SMM environment into an SMM environment.
  @param[in,out] CommBufferSize  The size of the CommBuffer.
**/
EFI_STATUS
EFIAPI
RWVariableHandler (
  IN     EFI_HANDLE    DispatchHandle,
  IN     CONST VOID    *Context,
  IN OUT VOID          *CommBuffer,
  IN OUT UINTN         *CommBufferSize
  )
{
  EFI_STATUS        Status         = EFI_SUCCESS;
  RW_VARIABLE_INFO  *CommData      = NULL;
  CHAR16            *VariableName  = NULL;
  VOID              *DataContent   = NULL;
  UINTN             DataSize       = 0x00;

  if (CommBuffer == NULL || CommBufferSize == NULL) {
    DEBUG((DEBUG_INFO, "%a: Invalid CommBuffer or CommBufferSize\n"));
    return EFI_INVALID_PARAMETER;
  }
  
  DataSize = *CommBufferSize;

  if (DataSize > sizeof(RW_VARIABLE_INFO)) {
   // DEBUG((DEBUG_INFO, "%a: Data size exceeds structure boundaries !!!\n", __func__));
    return EFI_INVALID_PARAMETER;
  }
  Status = gSmst->SmmAllocatePool (
            EfiRuntimeServicesData,
            DataSize,
            (VOID **)&CommData
            );
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a: Failed to allocate buffer\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }
  // Perform work on copied, user-supplied data.
  //DuplicateMemory(CommData, CommBuffer, DataSize);
  UINT32 dataOffset;
   UINT8  data;
  CommData = malloc(sizeof(RW_VARIABLE_INFO));
  klee_make_symbolic(&dataOffset, sizeof(dataOffset), "CommBuffer->DataOffset");
  CommData->DataOffset = dataOffset;
  klee_make_symbolic(&data, sizeof(data), "CommBuffer->data");
  klee_make_symbolic(&VariableName, sizeof(VariableName), "VariableName");
  //VariableName = (CHAR16 *)(&(CommData->Data));
  DataContent = (void *)((UINTN)VariableName + CommData->DataOffset);
  klee_print_expr("DataContent",DataContent);
  klee_print_expr("VariableName",VariableName);
  klee_print_expr("MAX_LENGTH",MAX_LENGTH);
  klee_assert(!PointerOutofBounds((UINT64)DataContent, (UINT64)(VariableName), MAX_LENGTH));

  // if ((UINT64)DataContent >= (UINT64)(VariableName + MAX_LENGTH)) {
  //  //DEBUG((DEBUG_ERROR, "%a: Data offset out of range !!!\n", __func__));
  //   //gSmst->SmmFreePool(CommData);
  //   return EFI_BAD_BUFFER_SIZE;
  // }




  if (CommData->AccessType & (READ_ATTRS | READ_DEF_ATTR)) {
    klee_print_expr("DataContent",DataContent);
    klee_print_expr("CommData->DataLength",CommData->DataLength);
    klee_print_expr("DataSize",DataSize);
    klee_assert(!PointerOutofBounds((UINTN)DataContent, CommData->DataLength, DataSize));
    // Status = AccessReadVariable (
    //           VariableName,
    //           &(CommData->VariableGuid),
    //           &(CommData->Attributes),
    //           &(CommData->DataLength),
    //           DataContent
    //           );
    if (!EFI_ERROR (Status)) {
      DataSize += CommData->DataLength;
    }
  } else if (CommData->AccessType & (WRITE_DEF_ATTR | WRITE_ATTRS)) {
    Status = AccessWriteVariable (
              VariableName,
              &(CommData->VariableGuid),
              CommData->Attributes,
              CommData->DataLength,
              DataContent
              );  
  } else {
    DEBUG((DEBUG_ERROR, "%a: Unsupported access type\n"));
    return EFI_INVALID_PARAMETER;
  }

  CommData->Status = Status;

  DuplicateMemory (CommBuffer, CommData, DataSize);
  return EFI_SUCCESS;
}

/**
  This function performs the read operation on NVRAM variables.

  @param[in]       VariableName  A Null-terminated string that is the name of the vendor's variable.
  @param[in]       VendorGuid    A unique identifier for the vendor.
  @param[out]      Attributes    If not NULL, a pointer to the memory location to return the attributes
                                 bitmask for the variable.
  @param[in, out]  DataSize      On input, the size in bytes of the return Data buffer. On output the
                                 size of data returned in Data.
  @param[out]      Data          The buffer to return the contents of the variable. May be NULL with a
                                 zero DataSize in order to determine the size buffer needed.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_NOT_FOUND          The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL   The DataSize is too small for the result.
  @retval EFI_INVALID_PARAMETER  VariableName, VendorGuid, or DataSize is NULL.
  @retval EFI_INVALID_PARAMETER  The DataSize is not too small and Data is NULL.
  @retval EFI_DEVICE_ERROR       The variable could not be retrieved due to a hardware error.
**/
// EFI_STATUS
// EFIAPI
// AccessReadVariable (
//   IN     CHAR16                    *VariableName,
//   IN     EFI_GUID                  *VendorGuid,
//   OUT    UINT32                    *Attributes     OPTIONAL,
//   IN OUT UINTN                     *DataSize,
//   OUT    VOID                      *Data           OPTIONAL
//   )
// {
//   return mSmmVariable->SmmGetVariable (VariableName, VendorGuid, NULL, DataSize, Data);
// }

/**
  @param[in]  VariableName       A Null-terminated string that is the name of the vendor's variable.
  @param[in]  VendorGuid         A unique identifier for the vendor.
  @param[in]  Attributes         Attributes bitmask to set for the variable.
  @param[in]  DataSize           The size in bytes of the Data buffer. Unless the EFI_VARIABLE_APPEND_WRITE
                                 or EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute is set, a
                                 size of zero causes the variable to be deleted.
  @param[in]  Data               The contents for the variable.

  @retval EFI_SUCCESS            The firmware has successfully stored the variable and its data.
  @retval EFI_INVALID_PARAMETER  An invalid combination of attribute bits, name, and GUID was supplied, or
                                 the DataSize exceeds the maximum allowed.
  @retval EFI_INVALID_PARAMETER  VariableName is an empty string.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be retrieved due to a hardware error.
  @retval EFI_WRITE_PROTECTED    The variable in question is read-only.
  @retval EFI_WRITE_PROTECTED    The variable in question cannot be deleted.
  @retval EFI_NOT_FOUND          The variable trying to be updated or deleted was not found.
**/
EFI_STATUS
EFIAPI
AccessWriteVariable (
  IN  CHAR16                       *VariableName,
  IN  EFI_GUID                     *VendorGuid,
  IN  UINT32                       Attributes,
  IN  UINTN                        DataSize,
  IN  VOID                         *Data
  )
{
  return mSmmVariable->SmmSetVariable (VariableName, VendorGuid, 0x7, DataSize, Data);
}

/**
  The module Entry Point of the Access Read/Write Variable SMM driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS        The entry point is executed successfully.
  @retval Other              Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
AccessRWVarEntryPoint (
  IN EFI_HANDLE                    ImageHandle,
  IN EFI_SYSTEM_TABLE              *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  DispatchHandle;
  EFI_HANDLE  ProtocolHandle;

  // 
  // Get mSmmVariable to get/set variable functions.
  //
  Status = gMmst->MmLocateProtocol(
                  &gEfiSmmVariableProtocolGuid, 
                  NULL,
                  (VOID **)&mSmmVariable
                  );
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a: Missing required SmmVariable protocol - %r\n", __func__, Status));
    return Status;
  }

  //
  //
  // Define the protocol functions
  if (FeaturePcdGet(PcdUseAccessRWVariable)) {
    gAccessRWVar.Read    = AccessReadVariable;
    gAccessRWVar.Write   = AccessWriteVariable;
  } else {
    DEBUG((DEBUG_INFO, "%a: Pcd flag not set. Abandoning module init\n"));
    return EFI_UNSUPPORTED;
  }

  //
  //
  // Install protocol
  ProtocolHandle = (EFI_HANDLE)NULL;
  Status = gMmst->MmInstallProtocolInterface (
                    &ProtocolHandle,
                    &gEfiAccessRWVarProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &gAccessRWVar
                    );
  ASSERT_EFI_ERROR (Status);

  //
  //
  // Install the Smi Handler
  DispatchHandle = (EFI_HANDLE)NULL;
  Status = gMmst->MmiHandlerRegister(
                    RWVariableHandler,
                    &gEfiRWVariableCommGuid,
                    &DispatchHandle
                    );
  ASSERT_EFI_ERROR (Status);

  return Status;
}