/** @file
  This file implements the Smm Spi functionality.

**/

#include "SmmSpiCommon.h"
#include "DxeSpi.h"
EFI_HANDLE mSmiHandle;

/**
  Communication service SMI Handler entry.

  This SMI handler provides services for Spi communication.

  Caution: This function may receive untrusted input.
  Communicate buffer and buffer size are external input, so this function will do basic validation.

  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     RegisterContext Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in, out] CommBuffer     A pointer to a collection of data in memory that will
                                 be conveyed from a non-MM environment into an MM environment.
  @param[in, out] CommBufferSize The size of the CommBuffer.

  @retval EFI_SUCCESS                         The interrupt was handled and quiesced. No other handlers
                                              should still be called.
**/
EFI_STATUS
EFIAPI
SmmSpiHandler (
  IN     EFI_HANDLE    DispatchHandle,
  IN     CONST VOID    *RegisterContext,
  IN OUT VOID          *CommBuffer,
  IN OUT UINTN         *CommBufferSize
  )
{
  EFI_STATUS           Status = EFI_ACCESS_DENIED;
  SMM_SPI_READ_DATA    *SmmSpiReadDataIn;
  SMM_SPI_READ_DATA    *SmmSpiReadDataOut;
  SMM_SPI_WRITE_DATA   *SmmSpiWriteDataIn;
  SMM_SPI_WRITE_DATA   *SmmSpiWriteDataOut;
  SPI_COMM_HEADER      *SpiCommHeader;
  SPI_COMM_HEADER      *ReturnComms = (SPI_COMM_HEADER *)CommBuffer;
  UINTN                TempCommBufferSize;
  UINTN                DataSize;
  // UINT8                *Buffer;

  //
  // If input is invalid, stop processing this SMI
  //
  if ((CommBuffer == NULL) || (CommBufferSize == NULL)) {
    return EFI_SUCCESS;
  }

  TempCommBufferSize = *CommBufferSize;

  if (TempCommBufferSize < SPI_COMM_HDR_SIZE) {
    DEBUG ((DEBUG_ERROR, "%a: SMM communication buffer size invalid!\n", __FUNCTION__));
    return EFI_SUCCESS;
  }

  if (!SmmIsBufferOutsideSmmValid ((UINTN)CommBuffer, TempCommBufferSize)) {
    DEBUG ((DEBUG_ERROR, "%a: SMM communication buffer in SMRAM or overflow!\n", __FUNCTION__));
    return EFI_SUCCESS;
  }

  SpiCommHeader = (SPI_COMM_HEADER *)AllocateRuntimeZeroPool(TempCommBufferSize);
  if (SpiCommHeader == NULL) {
    ReturnComms->ReturnStatus = EFI_OUT_OF_RESOURCES;
    return EFI_SUCCESS;
  }
  CopyMem (SpiCommHeader, CommBuffer, TempCommBufferSize);

  DataSize = TempCommBufferSize - SPI_COMM_HDR_SIZE;

  switch (SpiCommHeader->Function) {
    case FUNCTION_SPIFLASH_READ:
      SmmSpiReadDataOut = (SMM_SPI_READ_DATA *) ReturnComms->Data;
      SmmSpiReadDataIn = (SMM_SPI_READ_DATA *) SpiCommHeader->Data;
      SmmSpiReadDataIn->Buffer = (UINT8 *)(SmmSpiReadDataIn + 1);

      if (DataSize < sizeof (SMM_SPI_READ_DATA)) {
        DEBUG ((DEBUG_ERROR, "%a: SmmSpiReadData buffer size invalid!\n", __FUNCTION__));
        Status = EFI_ACCESS_DENIED;
        break;
      }
      if ((SmmSpiReadDataOut->Buffer + SmmSpiReadDataOut->ByteCount) > (ReturnComms->Data + DataSize)) {
        DEBUG ((DEBUG_ERROR, "%a: SmmSpiReadData buffer size invalid!\n", __FUNCTION__));
        Status = EFI_ACCESS_DENIED;
        break;
      }
      if ((SmmSpiReadDataOut->Buffer) != (UINT8 *)(SmmSpiReadDataOut + 1)) {
        DEBUG ((DEBUG_ERROR, "%a: SmmSpiReadData buffer location invalid!\n", __FUNCTION__));
        Status = EFI_ACCESS_DENIED;
        break;
      }

      Status = SpiFlashRead (
                SmmSpiReadDataIn->FlashRegionType,
                SmmSpiReadDataIn->Address,
                SmmSpiReadDataIn->ByteCount,
                SmmSpiReadDataIn->Buffer
                );

      if (!EFI_ERROR (Status)) {
        CopyMem (SmmSpiReadDataOut->Buffer, SmmSpiReadDataIn->Buffer, SmmSpiReadDataOut->ByteCount);
        Status = EFI_SUCCESS;
      }
    break;

    case FUNCTION_SPIFLASH_WRITE:
      SmmSpiWriteDataOut = (SMM_SPI_WRITE_DATA *) ReturnComms->Data;
      SmmSpiWriteDataIn = (SMM_SPI_WRITE_DATA *) SpiCommHeader->Data;
      SmmSpiWriteDataIn->Buffer = (UINT8 *)(SmmSpiWriteDataIn + 1);

      if (DataSize < sizeof (SMM_SPI_WRITE_DATA)) {
        DEBUG ((DEBUG_ERROR, "%a: SmmSpiWriteData buffer size invalid!\n", __FUNCTION__));
        Status = EFI_ACCESS_DENIED;
        break;
      }
      if ((SmmSpiWriteDataOut->Buffer + SmmSpiWriteDataOut->ByteCount) > (ReturnComms->Data + DataSize)) {
        DEBUG ((DEBUG_ERROR, "%a: SmmSpiWriteData buffer size invalid!\n", __FUNCTION__));
        Status = EFI_ACCESS_DENIED;
        break;
      }
      if ((SmmSpiWriteDataOut->Buffer) != (UINT8 *)(SmmSpiWriteDataOut + 1)) {
        DEBUG ((DEBUG_ERROR, "%a: SmmSpiWriteData buffer location invalid!\n", __FUNCTION__));
        Status = EFI_ACCESS_DENIED;
        break;
      }

      Status = SpiFlashWrite (
                SmmSpiWriteDataIn->FlashRegionType,
                SmmSpiWriteDataIn->Address,
                SmmSpiWriteDataIn->ByteCount,
                SmmSpiWriteDataIn->Buffer
                );
    break;

    default:
      Status = EFI_UNSUPPORTED;
  }

  ReturnComms->ReturnStatus = Status;
  FreePool(SpiCommHeader);

  return EFI_SUCCESS;
}

/**
  SMM END_OF_DXE protocol notification event handler.

  @param  Protocol   Points to the protocol's unique identifier
  @param  Interface  Points to the interface instance
  @param  Handle     The handle on which the interface was installed

  @retval EFI_SUCCESS   SmmEndOfDxeCallback runs successfully

**/
EFI_STATUS
EFIAPI
MmEndOfDxeCallback (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS Status = Status;
  // Unregister the SMI Handler
  if (mSmiHandle != NULL) {
    Status = gMmst->MmiHandlerUnRegister(mSmiHandle);
    ASSERT_EFI_ERROR (Status);
    mSmiHandle = NULL;
  }

  return Status;
}

/**
  The module Entry Point of the Spi SMM Driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Others         Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
SmmSpiEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS Status;
  VOID       *MmEndOfDxeRegistration;

  //
  // Register SMI handler.
  //
  mSmiHandle = NULL;
  Status = gMmst->MmiHandlerRegister (
                    SmmSpiHandler, 
                    &gEfiDxeSmmSpiProtocolGuid, 
                    &mSmiHandle
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Register EFI_SMM_END_OF_DXE_PROTOCOL_GUID notify function.
  //
  Status = gMmst->MmRegisterProtocolNotify (
                    &gEfiMmEndOfDxeProtocolGuid,
                    MmEndOfDxeCallback,
                    &MmEndOfDxeRegistration
                    );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
