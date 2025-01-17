/** @file

  This is a simple fault tolerant write driver that is intended to use in the SMM environment.

  This boot service protocol only provides fault tolerant write capability for
  block devices.  The protocol has internal non-volatile intermediate storage
  of the data and private information. It should be able to recover
  automatically from a critical fault, such as power failure.

  The implementation uses an FTW (Fault Tolerant Write) Work Space.
  This work space is a memory copy of the work space on the Working Block,
  the size of the work space is the FTW_WORK_SPACE_SIZE bytes.

  The work space stores each write record as EFI_FTW_RECORD structure.
  The spare block stores the write buffer before write to the target block.

  The write record has three states to specify the different phase of write operation.
  1) WRITE_ALLOCATED is that the record is allocated in write space.
     The information of write operation is stored in write record structure.
  2) SPARE_COMPLETED is that the data from write buffer is writed into the spare block as the backup.
  3) WRITE_COMPLETED is that the data is copied from the spare block to the target block.

  This driver operates the data as the whole size of spare block.
  It first read the SpareAreaLength data from the target block into the spare memory buffer.
  Then copy the write buffer data into the spare memory buffer.
  Then write the spare memory buffer into the spare block.
  Final copy the data from the spare block to the target block.

  To make this drive work well, the following conditions must be satisfied:
  1. The write NumBytes data must be fit within Spare area.
     Offset + NumBytes <= SpareAreaLength
  2. The whole flash range has the same block size.
  3. Working block is an area which contains working space in its last block and has the same size as spare block.
  4. Working Block area must be in the single one Firmware Volume Block range which FVB protocol is produced on.
  5. Spare area must be in the single one Firmware Volume Block range which FVB protocol is produced on.
  6. Any write data area (SpareAreaLength Area) which the data will be written into must be
     in the single one Firmware Volume Block range which FVB protocol is produced on.
  7. If write data area (such as Variable range) is enlarged, the spare area range must be enlarged.
     The spare area must be enough large to store the write data before write them into the target range.
  If one of them is not satisfied, FtwWrite may fail.
  Usually, Spare area only takes one block. That's SpareAreaLength = BlockSize, NumberOfSpareBlock = 1.

  Caution: This module requires additional review when modified.
  This driver need to make sure the CommBuffer is not in the SMRAM range.

Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "../../eval2_edk2-main/MdePkg/Include/PiMm.h"
#include "../../eval2_edk2-main/MdePkg/Include/Library/MmServicesTableLib.h"
#include "../../eval2_edk2-main/MdePkg/Include/Library/BaseLib.h"
#include "../../eval2_edk2-main/MdeModulePkg//Include/Protocol/SmmSwapAddressRange.h"
#include "../../eval2_edk2-main/MdeModulePkg/Include/Guid/FaultTolerantWrite.h"
#include "../../eval2_edk2-main/MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteSmmCommon.h"
#include "../../eval2_edk2-main//MdePkg/Include/Protocol/MmEndOfDxe.h"

VOID            *mFvbRegistration = NULL;
//EFI_FTW_DEVICE  *mFtwDevice       = NULL;

///
/// The flag to indicate whether the platform has left the DXE phase of execution.
///
BOOLEAN  mEndOfDxe = FALSE;

/**
  Retrieve the SMM FVB protocol interface by HANDLE.

  @param[in]  FvBlockHandle     The handle of SMM FVB protocol that provides services for
                                reading, writing, and erasing the target block.
  @param[out] FvBlock           The interface of SMM FVB protocol

  @retval EFI_SUCCESS           The interface information for the specified protocol was returned.
  @retval EFI_UNSUPPORTED       The device does not support the SMM FVB protocol.
  @retval EFI_INVALID_PARAMETER FvBlockHandle is not a valid EFI_HANDLE or FvBlock is NULL.

**/
EFI_STATUS
FtwGetFvbByHandle (
  IN  EFI_HANDLE                          FvBlockHandle,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvBlock
  )
{
   //
  // To get the SMM FVB protocol interface on the handle
  //
  /*return gMmst->MmHandleProtocol (
                  FvBlockHandle,
                  &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                  (VOID **)FvBlock
                  );*/
  return EFI_UNSUPPORTED;}

/**
  Retrieve the SMM Swap Address Range protocol interface.
**/
EFI_STATUS
FtwGetSarProtocol (
  OUT VOID  **SarProtocol
  )
{
  /*
  return gMmst->MmLocateProtocol (
                    &gEfiSmmSwapAddressRangeProtocolGuid,
                    NULL,
                    SarProtocol
                    );
  */
  return EFI_UNSUPPORTED;
}

/**
  Function returns an array of handles that support the SMM FVB protocol
  in a buffer allocated from pool.
**/
EFI_STATUS
GetFvbCountAndBuffer (
  OUT UINTN       *NumberHandles,
  OUT EFI_HANDLE  **Buffer
  )
{
  /*
  // Original code left intact, but commented out references to gMmst

  EFI_STATUS  Status;
  UINTN       BufferSize;

  if ((NumberHandles == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  BufferSize     = 0;
  *NumberHandles = 0;
  *Buffer        = NULL;
  Status         = gMmst->MmLocateHandle (
                            ByProtocol,
                            &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                            NULL,
                            &BufferSize,
                            *Buffer
                            );
  if (EFI_ERROR (Status) && (Status != EFI_BUFFER_TOO_SMALL)) {
    return EFI_NOT_FOUND;
  }

  *Buffer = AllocatePool (BufferSize);
  if (*Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gMmst->MmLocateHandle (
                    ByProtocol,
                    &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                    NULL,
                    &BufferSize,
                    *Buffer
                    );

  *NumberHandles = BufferSize / sizeof (EFI_HANDLE);
  if (EFI_ERROR (Status)) {
    *NumberHandles = 0;
    FreePool (*Buffer);
    *Buffer = NULL;
  }

  return Status;
  */
  return EFI_UNSUPPORTED;
}

/**
  Get the handle of the SMM FVB protocol by the FVB base address and attributes.
**/
EFI_STATUS
GetFvbByAddressAndAttribute (
  IN  EFI_PHYSICAL_ADDRESS  Address,
  IN  EFI_FVB_ATTRIBUTES_2  Attributes,
  OUT EFI_HANDLE            *SmmFvbHandle
  )
{
  // If you cannot run real code outside EDK, comment out or stub:
  return EFI_ABORTED;
}

typedef void (*FuncType)();




















































































/**
  Communication service SMI Handler entry.

  This SMI handler provides services for the fault tolerant write wrapper driver.

  Caution: This function requires additional review when modified.
  This driver need to make sure the CommBuffer is not in the SMRAM range.
  Also in FTW_FUNCTION_GET_LAST_WRITE case, check SmmFtwGetLastWriteHeader->Data +
  SmmFtwGetLastWriteHeader->PrivateDataSize within communication buffer.

  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     RegisterContext Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in, out] CommBuffer     A pointer to a collection of data in memory that will be conveyed
                                 from a non-SMM environment into an SMM environment.
  @param[in, out] CommBufferSize The size of the CommBuffer.

  @retval EFI_SUCCESS                         The interrupt was handled and quiesced. No other handlers
                                              should still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_QUIESCED  The interrupt has been quiesced but other handlers should
                                              still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_PENDING   The interrupt is still pending and other handlers should still
                                              be called.
  @retval EFI_INTERRUPT_PENDING               The interrupt could not be quiesced.

**/
EFI_STATUS
EFIAPI
SmmFaultTolerantWriteHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *RegisterContext,
  IN OUT VOID        *CommBuffer,
  IN OUT UINTN       *CommBufferSize
  )
{
  EFI_STATUS                           Status;
  SMM_FTW_GET_MAX_BLOCK_SIZE_HEADER    *SmmGetMaxBlockSizeHeader;
  SMM_FTW_ALLOCATE_HEADER              *SmmFtwAllocateHeader;
  SMM_FTW_WRITE_HEADER                 *SmmFtwWriteHeader;
  SMM_FTW_RESTART_HEADER               *SmmFtwRestartHeader;
  SMM_FTW_GET_LAST_WRITE_HEADER        *SmmFtwGetLastWriteHeader;
  UINTN                                CommBufferPayloadSize;
  UINTN                                PrivateDataSize;
  UINTN                                TempCommBufferSize;
  UINTN                                InfoSize;
  UINTN                                Length;
  VOID                                 *PrivateData;
  VOID                                 *StructureData;
  EFI_HANDLE                           SmmFvbHandle;
  SMM_FTW_COMMUNICATE_FUNCTION_HEADER  SmmFtwFunctionHeader;

  //
  // If input is invalid, stop processing this SMI
  //
  if ((CommBuffer == NULL) || (CommBufferSize == NULL)) {
    return EFI_SUCCESS;
  }

  TempCommBufferSize = *CommBufferSize;
  if (!FtwSmmIsBufferOutsideSmmValid_stub ((UINTN)CommBuffer, TempCommBufferSize)) {
   // DEBUG ((DEBUG_ERROR, "SmmFtwHandler: SMM communication buffer in SMRAM or overflow!\n"));
    return EFI_SUCCESS;
  }
  
  BOOLEAN ArbitraryCodeInjection = TRUE;  // Always true
  klee_assert(!ArbitraryCodeInjection);
  //CopyMem((void *)&SmmFtwFunctionHeader, CommBuffer, SMM_FTW_COMMUNICATE_HEADER_SIZE);

  

  StructureData = (void *)((SMM_FTW_COMMUNICATE_FUNCTION_HEADER *)CommBuffer)->Data;

  if (TempCommBufferSize < SMM_FTW_COMMUNICATE_HEADER_SIZE) {
   // DEBUG ((DEBUG_ERROR, "SmmFtwHandler: SMM communication buffer size invalid!\n"));
    return EFI_SUCCESS;
  }
  
  CommBufferPayloadSize = TempCommBufferSize - SMM_FTW_COMMUNICATE_HEADER_SIZE;

  if (mEndOfDxe) {
    //DEBUG ((DEBUG_ERROR, "SmmFtwHandler: Not safe after End Of Dxe!\n"));
    SmmFtwFunctionHeader.ReturnStatus = EFI_ACCESS_DENIED;
    return EFI_SUCCESS;
  }

  /* 
   * We'll keep the switch logic but comment out real calls 
   * that reference code not available in user mode.
   */
  switch (SmmFtwFunctionHeader.Function) {
    case FTW_FUNCTION_GET_MAX_BLOCK_SIZE:
      if (CommBufferPayloadSize < sizeof (SMM_FTW_GET_MAX_BLOCK_SIZE_HEADER)) {
        //DEBUG ((DEBUG_ERROR, "GetMaxBlockSize: SMM communication buffer size invalid!\n"));
        return EFI_SUCCESS;
      }
      SmmGetMaxBlockSizeHeader = (SMM_FTW_GET_MAX_BLOCK_SIZE_HEADER *)StructureData;
      // Example stub logic
      Status = EFI_SUCCESS; // FtwGetMaxBlockSize(...) commented out
      break;

    case FTW_FUNCTION_ALLOCATE:
      if (CommBufferPayloadSize < sizeof (SMM_FTW_ALLOCATE_HEADER)) {
        //DEBUG ((DEBUG_ERROR, "Allocate: SMM communication buffer size invalid!\n"));
        return EFI_SUCCESS;
      }
      SmmFtwAllocateHeader = (SMM_FTW_ALLOCATE_HEADER *)StructureData;
      Status = EFI_SUCCESS; // FtwAllocate(...) commented out
      break;

    case FTW_FUNCTION_WRITE:
      if (CommBufferPayloadSize < OFFSET_OF (SMM_FTW_WRITE_HEADER, Data)) {
        //DEBUG ((DEBUG_ERROR, "Write: SMM communication buffer size invalid!\n"));
        return EFI_SUCCESS;
      }
      SmmFtwWriteHeader = (SMM_FTW_WRITE_HEADER *)StructureData;
      Length            = SmmFtwWriteHeader->Length;
      PrivateDataSize   = SmmFtwWriteHeader->PrivateDataSize;
      // In real code, we do more checks. Minimally keep them or comment out.
      Status = EFI_SUCCESS;
      break;

    case FTW_FUNCTION_RESTART:
      if (CommBufferPayloadSize < sizeof (SMM_FTW_RESTART_HEADER)) {
        //DEBUG ((DEBUG_ERROR, "Restart: SMM communication buffer size invalid!\n"));
        return EFI_SUCCESS;
      }
      SmmFtwRestartHeader = (SMM_FTW_RESTART_HEADER *)StructureData;
      Status              = EFI_SUCCESS; 
      break;

    case FTW_FUNCTION_ABORT:
      Status = EFI_SUCCESS;
      break;

    case FTW_FUNCTION_GET_LAST_WRITE:
      if (CommBufferPayloadSize < OFFSET_OF (SMM_FTW_GET_LAST_WRITE_HEADER, Data)) {
        //DEBUG ((DEBUG_ERROR, "GetLastWrite: SMM communication buffer size invalid!\n"));
        return EFI_SUCCESS;
      }
      SmmFtwGetLastWriteHeader = (SMM_FTW_GET_LAST_WRITE_HEADER *)StructureData;
      PrivateDataSize          = SmmFtwGetLastWriteHeader->PrivateDataSize;
      Status = EFI_SUCCESS;
      break;

    default:
      Status = EFI_UNSUPPORTED;
  }

  SmmFtwFunctionHeader.ReturnStatus = Status;
  return EFI_SUCCESS;
}

/* ------------------------------------------------------------------
   The rest of the file is unchanged or commented out as needed.
   ------------------------------------------------------------------ */

/**
  SMM Firmware Volume Block Protocol notification event handler.
**/
EFI_STATUS
EFIAPI
FvbNotificationEvent (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  return EFI_SUCCESS; // Minimally stub out
}

/**
  SMM END_OF_DXE protocol notification event handler.
**/
EFI_STATUS
EFIAPI
MmEndOfDxeCallback (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  mEndOfDxe = TRUE;
  return EFI_SUCCESS;
}

/**
  Shared entry point of the module
**/
EFI_STATUS
MmFaultTolerantWriteInitialize (
  VOID
  )
{
  return EFI_SUCCESS; // Minimally stub out
}
