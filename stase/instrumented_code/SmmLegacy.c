/** @file
   Executing some code on the Smm Stack Driver.

**/
#include "SmmLegacy.h"
BOOLEAN StackIsExecutable = FALSE;
UINT32        ConfigurationFlags      = 0x00;
wchar_t        SmmLegacyVariableName[] = L"SmmLegacyConfiguration";
LIST_ENTRY    mSmmLegacyDispatchQueue = INITIALIZE_LIST_HEAD_VARIABLE (mSmmLegacyDispatchQueue);

EDKII_SMM_LEGACY_PROTOCOL  gSmmLegacy     = { NULL, NULL };
EFI_SMM_VARIABLE_PROTOCOL  *SmmVariable   = NULL;
EFI_HANDLE                 ProtocolHandle = NULL;
EFI_HANDLE                 SmiHandle      = NULL;

VOID                                 *LoadSmmVariableRegistration = NULL;
EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL  *mSmmMemoryAttribute1        = NULL;
typedef void (*FuncType)(); // CROM - smm stack exec

EFI_STATUS
EFIAPI
ClearStackNx (VOID)
{
  UINT64                Attribute   = 0x01;
  EFI_STATUS            Status      = EFI_SUCCESS;
//   EFI_PHYSICAL_ADDRESS  Address     = SmmStackArrayBase;
//   UINTN                *AttrPoint  = &Attribute;

//   BOOLEAN               CetEnabled;
//   BOOLEAN               WpEnabled;

//   DisableReadOnlyPageWriteProtect (&WpEnabled, &CetEnabled);
//   Status = mSmmMemoryAttribute1->GetMemoryAttributes (
//                                      mSmmMemoryAttribute1,
//                                      Address,
//                                      0x4000,
//                                      AttrPoint
//                                      );
//   if (Attribute == 0x00 || EFI_ERROR (Status)) {
//     return Status;
//   }

//   Status = mSmmMemoryAttribute1->ClearMemoryAttributes (
//                                      mSmmMemoryAttribute1,
//                                      Address,
//                                      0x4000,
//                                      *AttrPoint
//                                      );
//   EnableReadOnlyPageWriteProtect (WpEnabled, CetEnabled);
  StackIsExecutable = TRUE;
  return Status;
}

EFI_STATUS
EFIAPI
SetStackNx (VOID)
{
  klee_assert(!StackIsExecutable);
  EFI_STATUS            Status;
  UINT64                Attribute = 0x4000;
//   EFI_PHYSICAL_ADDRESS  Address   = SmmStackArrayBase;
  
//   BOOLEAN               CetEnabled;
//   BOOLEAN               WpEnabled;
  
//   DisableReadOnlyPageWriteProtect (&WpEnabled, &CetEnabled);
//   Status = mSmmMemoryAttribute1->SetMemoryAttributes (
//                           mSmmMemoryAttribute1,
//                           Address,
//                           0x4000,
//                           Attribute
//                           );
//   EnableReadOnlyPageWriteProtect (WpEnabled, CetEnabled);
  StackIsExecutable = FALSE; 
  return Status;
}

/**
  Find SmmSwDispatch2Context by SwSmiInputValue.

  @param[in] SwSmiInputValue      The value to indentify the SmmSwDispatch2 context

  @return Pointer to EFI_SMM_LEGACY_DISPATCH_CONTEXT context
**/
EFI_STATUS
FindContextBySwSmiInputValue (
  IN UINTN                            InputValue,
  IN EFI_SMM_LEGACY_DISPATCH_CONTEXT  **StartDispatch
  )
{
  LIST_ENTRY                      *Node;
  EFI_SMM_LEGACY_DISPATCH_CONTEXT *Dispatch2Context;

  if (*StartDispatch == (EFI_SMM_LEGACY_DISPATCH_CONTEXT *)NULL) {
    Node = mSmmLegacyDispatchQueue.ForwardLink;
  } else {
    Node = GetNextNode(&mSmmLegacyDispatchQueue, &(*StartDispatch)->Link);
  }

  if (Node->ForwardLink == &(*StartDispatch)->Link) {
    (*StartDispatch) = (EFI_SMM_LEGACY_DISPATCH_CONTEXT *)NULL;
    return EFI_NOT_FOUND;
  }

  Node = mSmmLegacyDispatchQueue.ForwardLink;
  for ( ; Node != &mSmmLegacyDispatchQueue; Node = Node->ForwardLink) {
    Dispatch2Context = BASE_CR (Node, EFI_SMM_LEGACY_DISPATCH_CONTEXT, Link);
    if (Dispatch2Context->InputValue == InputValue) {
      *(StartDispatch) = Dispatch2Context;
      return EFI_SUCCESS;
    }
  }

  *(StartDispatch) = (EFI_SMM_LEGACY_DISPATCH_CONTEXT *)NULL;
  return EFI_NOT_FOUND;
}

/**
  Dispatch registered SMM handlers

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  RegisterContext Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmLegacyDispatcher (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *RegisterContext,
  IN OUT VOID        *CommBuffer,
  IN OUT UINTN       *CommBufferSize
  )
{
  EFI_STATUS                        Status;
  EFI_SMM_LEGACY_CONTEXT            CpuContext;
  EFI_SMM_LEGACY_DISPATCH_CONTEXT   *DispatchContext;
  EDKII_SMM_LEGACY_REGISTER_CONTEXT RegContext;
  EFI_SMM_HANDLER_ENTRY_POINT2      DispatchFunction;
  UINTN                             Size;

  Status = EFI_SUCCESS;
  CpuContext.CommandPort = IoRead8 (0xB3);

  //
  // Get the dispatch settings
  //
  Status = GetNvConfiguration();
 
  if (EFI_ERROR (Status)) {
    ConfigurationFlags = 0x07;
    DEBUG((DEBUG_INFO, "%a: Smm Legacy configuration fail. Using default 0x%08x\n", __func__, ConfigurationFlags));
  }
   
  //
  // These won't change between dispatches
  //
  Size                    = *CommBufferSize;
  RegContext.InputValue   = CpuContext.CommandPort;
  DispatchContext         = (EFI_SMM_LEGACY_DISPATCH_CONTEXT *)NULL;
    
  FindContextBySwSmiInputValue (CpuContext.CommandPort, &DispatchContext);
  // if (DispatchContext == (EFI_SMM_LEGACY_DISPATCH_CONTEXT *)NULL) {
  //  // DEBUG((DEBUG_ERROR, "No handlers for SMI value 0x%x\n", CpuContext.CommandPort));
  //   Status = EFI_NOT_FOUND;
  //   goto done;
  // }

  while (DispatchContext != (EFI_SMM_LEGACY_DISPATCH_CONTEXT *)NULL) {
    ClearStackNx();
    // Dispatch the handler
    DispatchFunction = (EFI_SMM_HANDLER_ENTRY_POINT2)DispatchContext->DispatchFunction;
    Status           = DispatchFunction (DispatchHandle, &RegContext, CommBuffer, &Size);

    if ((ConfigurationFlags & 0x2) == 0) {
      // Only dispatch every registered handle if the configuration
      // allows multiple dispatch.
      goto done;
    }
    SetStackNx();
    FindContextBySwSmiInputValue (CpuContext.CommandPort, &DispatchContext);
  }

done:
  return Status;
}

/**
  Register a child SMI source dispatch function for the specified software SMI.

  This service registers a function (DispatchFunction) which will be called when the software
  SMI source specified by RegContext->SwSmiCpuIndex is detected. On return, DispatchHandle
  contains a unique handle which may be used later to unregister the function using UnRegister().

  @param[in]  This               Pointer to the EFI_SMM_SW_DISPATCH2_PROTOCOL instance.
  @param[in]  DispatchFunction   Function to register for handler when the specified software
                                 SMI is generated.
  @param[in, out]  RegContext    Pointer to the dispatch function's context.
                                 The caller fills this context in before calling
                                 the register function to indicate to the register
                                 function which Software SMI input value the
                                 dispatch function should be invoked for.
  @param[out] DispatchHandle     Handle generated by the dispatcher to track the
                                 function instance.

  @retval EFI_SUCCESS            The dispatch function has been successfully
                                 registered and the SMI source has been enabled.
  @retval EFI_DEVICE_ERROR       The SW driver was unable to enable the SMI source.
  @retval EFI_INVALID_PARAMETER  RegisterContext is invalid. The SW SMI input value
                                 is not within valid range.
  @retval EFI_OUT_OF_RESOURCES   There is not enough memory (system or SMM) to manage this
                                 child.
  @retval EFI_OUT_OF_RESOURCES   A unique software SMI value could not be assigned
                                 for this dispatch.
**/
EFI_STATUS
EFIAPI
SmmLegacyRegister (
  IN  CONST EDKII_SMM_LEGACY_PROTOCOL           *This,
  IN        EFI_SMM_HANDLER_ENTRY_POINT2        DispatchFunction,
  IN  OUT   EDKII_SMM_LEGACY_REGISTER_CONTEXT   *RegContext,
  OUT       EFI_HANDLE                          *DispatchHandle
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  EFI_SMM_LEGACY_DISPATCH_CONTEXT  *Context;

  //
  // Register
  //
  Status = gSmst->SmmAllocatePool (EfiRuntimeServicesData, sizeof (*Context), (VOID **)&Context);
 // ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  *DispatchHandle           = (EFI_HANDLE)Context;
  Context->Signature        = SMM_LEGACY_HANDLER_SIGNATURE;
  Context->InputValue       = RegContext->InputValue;
  Context->DispatchFunction = (UINTN)DispatchFunction;
  Context->DispatchHandle   = *DispatchHandle;
  InsertTailList (&mSmmLegacyDispatchQueue, &Context->Link);
  return Status;
}

/**
  Unregister a child SMI source dispatch function for the specified software SMI.

  This service removes the handler associated with DispatchHandle so that it will no longer be
  called in response to a software SMI.

  @param[in] This                Pointer to the EFI_SMM_SW_DISPATCH2_PROTOCOL instance.
  @param[in] DispatchHandle      Handle of dispatch function to deregister.

  @retval EFI_SUCCESS            The dispatch function has been successfully unregistered.
  @retval EFI_INVALID_PARAMETER  The DispatchHandle was not valid.
**/
EFI_STATUS
EFIAPI
SmmLegacyUnRegister (
  IN CONST EDKII_SMM_LEGACY_PROTOCOL    *This,
  IN       EFI_HANDLE                   DispatchHandle
  )
{
  return EFI_SUCCESS;
}

/**
 
**/
EFI_STATUS
GetNvConfiguration (
  VOID
  )
{
  UINTN           DataSize;
  EFI_STATUS      Status;
  UINT32          Attributes;
  UINT32          Data;

  Data        = 0x00;
  Attributes  = 0x00;
  DataSize    = sizeof(Data);

  // Assign the stubbed protocol
  if (SmmVariable == NULL) {
    SmmVariable = &StubSmmVariableProtocol;
  }
  
  Status      = SmmVariable->SmmGetVariable (
                  SmmLegacyVariableName,
                  &gEfiSmmLegacyVariableGuid,
                  &Attributes,
                  &DataSize,
                  &Data
                  );
  if (!EFI_ERROR (Status)) {
    goto done;
  }

  DEBUG((DEBUG_INFO, "No SmmLegacyConfiguration variable. Creating.\n"));

  Data        = 0x07;
  Status      = SmmVariable->SmmSetVariable (
                        SmmLegacyVariableName,
                        &gEfiSmmLegacyVariableGuid,
                        (EFI_VARIABLE_BOOTSERVICE_ACCESS |
                        EFI_VARIABLE_RUNTIME_ACCESS | 
                        EFI_VARIABLE_NON_VOLATILE),
                        sizeof(ConfigurationFlags), 
                        (VOID **)(&Data)
                        );                  
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_INFO, "Smm Legacy failed to create configuration variable\n"));
    Data = 0x07;
  }

done:
  ConfigurationFlags = Data;
  return EFI_SUCCESS;
}

/**

**/
VOID
SmmLegacyExitPoint ( 
  VOID
  )
{
  if (SmiHandle != (EFI_HANDLE)NULL) {
    gSmst->SmiHandlerUnRegister(&SmiHandle);
  }
  
  if (ProtocolHandle != (EFI_HANDLE)NULL) {
    gSmst->SmmUninstallProtocolInterface(
            &ProtocolHandle,
            &gEfiSmmLegacyProtocolGuid,
            &gSmmLegacy
            );
  }
}

EFI_STATUS
EFIAPI
LoadSmmVariableProtocolCallback (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS      Status;

  Status = gSmst->SmmLocateProtocol (&gEfiSmmVariableProtocolGuid, NULL, (VOID **)&SmmVariable);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  The module Entry Point of the SMM driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS        The entry point is executed successfully.
  @retval Other              Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
SmmLegacyEntryPoint (
  VOID
  )
{
  EFI_STATUS  Status    = EFI_SUCCESS;
  UINT32      PcdFlags  = 0x00;

  PcdFlags = PcdGet32(PcdSmmLegacyPropertyMask);
 
  if ((PcdFlags & 0x1) != 0) {

    Status = gMmst->MmRegisterProtocolNotify(
                    &gEfiSmmVariableProtocolGuid,
                    LoadSmmVariableProtocolCallback,
                    &LoadSmmVariableRegistration
                    );
    ASSERT_EFI_ERROR (Status);
    
    //
    // Get MemoryAttribute protocol
    //
    Status = gSmst->SmmLocateProtocol (
                      &gEdkiiSmmMemoryAttributeProtocolGuid, 
                      NULL, (VOID **)&mSmmMemoryAttribute1
                      );
    ASSERT_EFI_ERROR (Status);

    //
    // Register SMI handler function
    //
    Status = gMmst->MmiHandlerRegister (
                      SmmLegacyDispatcher,
                      &gEfiSmmLegacyCommGuid, 
                      &SmiHandle
                      );
    ASSERT_EFI_ERROR (Status);

    gSmmLegacy.Register = SmmLegacyRegister;
    gSmmLegacy.UnRegister = SmmLegacyUnRegister;

    //
    // Publish Smm Legacy Protocol
    //
    Status = gSmst->SmmInstallProtocolInterface (
                      &ProtocolHandle,
                      &gEfiSmmLegacyProtocolGuid,
                      EFI_NATIVE_INTERFACE,
                      &gSmmLegacy
                      );
    ASSERT_EFI_ERROR (Status);
  }

  DEBUG((DEBUG_INFO, "%a: %p\n", __func__, &SmmLegacyDispatcher));

  return Status;
}



  