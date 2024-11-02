// OvmfPkg/B2SmmDriver/B2SmmDriver.c

#include "../../eval2_edk2-main/MdePkg/Include/Uefi.h"
#include "../../eval2_edk2-main/MdePkg/Include/Library/UefiBootServicesTableLib.h"
#include "../../eval2_edk2-main/MdePkg/Include/Library/UefiRuntimeServicesTableLib.h"
#include "../../eval2_edk2-main/MdePkg/Include/Library/UefiLib.h"
#include "../../eval2_edk2-main/MdePkg/Include/Library/DebugLib.h"
#include "../../eval2_edk2-main/MdePkg/Include/Library/MemoryAllocationLib.h"
#include "../../eval2_edk2-main/MdePkg/Include/Library/BaseMemoryLib.h"
#include "../../eval2_edk2-main/MdePkg/Include/Library/SmmServicesTableLib.h"
#include "../../eval2_edk2-main/MdeModulePkg/Include/Protocol/SmmVariable.h"
#include "../../eval2_edk2-main/MdePkg/Include/Protocol/SmmBase2.h"
#include "../../eval2_edk2-main/MdePkg/Include/Protocol/SmmSwDispatch2.h"
#include "../../eval2_edk2-main/MdePkg/Include/Protocol/SmmCpu.h"
#include "../../eval2_edk2-main/MdePkg/Include/Protocol/MpService.h"
#include "../../eval2_edk2-main/MdePkg/Include/Protocol/MmCpu.h"
#include "../../eval2_edk2-main/MdePkg/Include/Library/SmmMemLib.h"

#define MAX_VALUES 2
int OffByOne(UINTN dataPort, UINTN maxValue) {
    
    if (dataPort == maxValue) {
        return 1;
    }
    else 
      return 0;
}

EFI_GUID gB2SmiErrorTypeVariableGuid  = { 0x04147029, 0x2849, 0xC7EA, { 0xFF, 0x15, 0xDD, 0x38, 0x1E, 0x38, 0x78, 0xCA } };
EFI_GUID gB2SmiErrorCountVariableGuid = { 0x9DC6C3D2, 0xF870, 0x7048, { 0xB7, 0xD5, 0xB1, 0x66, 0x67, 0x00, 0xD1, 0xF4 } };

typedef UINTN B2_ERROR_COUNT;
typedef UINTN B2_ERROR_TYPES[MAX_VALUES];

typedef struct {
  B2_ERROR_TYPES error_types;
  B2_ERROR_COUNT failures;
} B2_DATA_STORAGE;

/**
  This function fetches the B2 error types.

  @param[out] Data              The location to write the error types data

  @retval     EFI_SUCCESS       The error types variable was successfully retrieved.
  @retval     Others            Other errors as indicated.
**/
EFI_STATUS
GetErrorTypes(
  OUT B2_ERROR_TYPES *Data
  )
{
    EFI_STATUS                Status;
    EFI_SMM_VARIABLE_PROTOCOL *SmmVariable;
    UINTN                     DataSize;
    
    Status = gSmst->SmmLocateProtocol(
                      &gEfiSmmVariableProtocolGuid,
                      NULL,
                      (void**)&SmmVariable
                      );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    DataSize = sizeof(B2_ERROR_TYPES);
    return SmmVariable->SmmGetVariable(
                          L"PowerStatus",
                          &gB2SmiErrorTypeVariableGuid,
                          NULL,
                          &DataSize,
                          Data
                          );
}


/**
  This function sets the B2 error types.

  @param[in]  Data              The error types data to write

  @retval     EFI_SUCCESS       The error type variable was successfully written.
  @retval     Others            Other errors as indicated.
**/
EFI_STATUS
SetErrorTypes(
  IN B2_ERROR_TYPES *Data
  )
{
  EFI_STATUS                Status;
  EFI_SMM_VARIABLE_PROTOCOL *SmmVariable;

  Status = gSmst->SmmLocateProtocol(
                    &gEfiSmmVariableProtocolGuid,
                    NULL,
                    (void**)&SmmVariable
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return SmmVariable->SmmSetVariable(
                      L"PowerStatus", &gB2SmiErrorTypeVariableGuid,
                      (EFI_VARIABLE_NON_VOLATILE |
                      EFI_VARIABLE_BOOTSERVICE_ACCESS |
                      EFI_VARIABLE_RUNTIME_ACCESS),
                      sizeof(B2_ERROR_TYPES), Data);
}

/**
  This function gets the B2 error count.

  @param[out] Data              The location to write the error count data.

  @retval     EFI_SUCCESS       The error count data was successfully retrieved.
  @retval     Others            Other errors as indicated.
**/
EFI_STATUS
GetErrorCount(
  OUT B2_ERROR_COUNT *Data
  )
{
    EFI_STATUS                Status;
    EFI_SMM_VARIABLE_PROTOCOL *SmmVariable;
    UINTN                     DataSize;

    Status = gSmst->SmmLocateProtocol(
                      &gEfiSmmVariableProtocolGuid,
                      NULL,
                      (void**)&SmmVariable
                      );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    DataSize = sizeof(B2_ERROR_COUNT);
    return SmmVariable->SmmGetVariable(
                          L"ErrorCount",
                          &gB2SmiErrorCountVariableGuid,
                          NULL,
                          &DataSize,
                          Data
                          );
}


/**
  This function sets the B2 error count.

  @param[out] Data              The value to write to the error count variable.

  @retval     EFI_SUCCESS       The error count data was successfully written.
  @retval     Others            Other errors as indicated.
**/
EFI_STATUS
SetErrorCount(
  B2_ERROR_COUNT *Data
  )
{
  EFI_STATUS                Status;
  EFI_SMM_VARIABLE_PROTOCOL *SmmVariable;
  
  Status = gSmst->SmmLocateProtocol(
                    &gEfiSmmVariableProtocolGuid,
                    NULL,
                    (void**)&SmmVariable
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return SmmVariable->SmmSetVariable(
                        L"ErrorCount",
                        &gB2SmiErrorCountVariableGuid,
                        (EFI_VARIABLE_NON_VOLATILE |
                        EFI_VARIABLE_BOOTSERVICE_ACCESS |
                        EFI_VARIABLE_RUNTIME_ACCESS),
                        sizeof(B2_ERROR_COUNT),
                        Data
                        );
}

/**
  This SMI handler provides access to the B2 Dispatcher.

  @param[in] DispatchHandle    The unique handle assigned to this handler by SmiHandlerRegister() 
  @param[in] Context           Points to an optional handler context which was specified when the
                               handler was registered.
  @param[in] CommBuffer        A pointer to a collection of data in memory that will be conveyed
                               from a non-SMM environment into an SMM environment.
  @param[in] CommBufferSize    The size of the CommBuffer

  @retval    EFI_SUCCESS       The interrupt was handled and quiesced.
  @retval    Others            Other errors as indicated.
**/
EFI_STATUS
EFIAPI
B2SmiHandler (
  IN EFI_HANDLE DispatchHandle,
  IN CONST VOID *Context OPTIONAL,
  IN OUT VOID *CommBuffer OPTIONAL,
  IN OUT UINTN *CommBufferSize OPTIONAL
  )
{
  EFI_SMM_SW_CONTEXT* data;
  B2_DATA_STORAGE     variables = {0};
  EFI_STATUS          Status;
   //Symbolic error_types
  klee_make_symbolic(&variables.error_types, sizeof(variables), "variables.error_types");
  if (CommBuffer != NULL && CommBufferSize != NULL && *CommBufferSize == sizeof(EFI_SMM_SW_CONTEXT)) {
    data = (EFI_SMM_SW_CONTEXT*) CommBuffer;
    klee_print_expr("data->DataPort",data->DataPort);
    klee_print_expr("MAX_VALUES",MAX_VALUES);
    klee_assert(!OffByOne(data->DataPort,MAX_VALUES));
    if (data->DataPort <= MAX_VALUES) {
      variables.error_types[data->DataPort] = 1;
      variables.failures++;
      if (variables.failures == 10) {
        //DEBUG((DEBUG_ERROR, "Error count exceeded: Halting Processor\n"));
        gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
      }
       
      Status = SetErrorTypes(&variables.error_types);
      if (EFI_ERROR (Status)) {
          DEBUG((DEBUG_ERROR, "Failed to set errorTypes: %r\n", Status));
      }

      Status = SetErrorCount(&variables.failures);
      if (EFI_ERROR (Status)) {
          DEBUG((DEBUG_ERROR, "Failed to set errorCount: %r\n", Status));
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  The module Entry Point of the B2 SMM driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS        The entry point is executed successfully.
  @retval Other              Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
B2SmmDriverEntry (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS                      Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL   *SwDispatch;
  EFI_SMM_SW_REGISTER_CONTEXT     SwContext;
  EFI_HANDLE                      DispatchHandle;

  Status = gSmst->SmmLocateProtocol(&gEfiSmmSwDispatch2ProtocolGuid, NULL, (VOID**)&SwDispatch);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "B2SmmDriver/B2SmmDriverEntry Failed to locate SMM SW Dispatch Protocol: %r\n", Status));
    return Status;
  }

  SwContext.SwSmiInputValue = 0xB2;
  Status = SwDispatch->Register(SwDispatch, B2SmiHandler, &SwContext, &DispatchHandle);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "B2SmmDriver/B2SmmDriverEntry Failed to register SMM SW handler: %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}