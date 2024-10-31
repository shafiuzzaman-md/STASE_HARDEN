/** @file
  This file implements the Dxe Spi protocol.

**/

#include "DxeSpi.h"
//
// Global variables
//
EFI_SMM_COMMUNICATION_PROTOCOL  *mSmmCommunication = NULL;

DXE_SPI_PROTOCOL  mDxeSpiProtocol = {
  DxeSpiRead,
  DxeSpiWrite,
};

/**
  Initialize the communicate buffer using DataSize and Function number.

  @param[out]      CommunicateBuffer The communicate buffer. Caller should free it after use.
  @param[out]      DataPtr           Points to the data in the communicate buffer. Caller should not free it.
  @param[in]       DataSize          The payload size.
  @param[in]       Function          The function number used to initialize the communicate header.
 
**/
VOID
InitCommunicateBuffer (
  OUT     VOID   **CommunicateBuffer,
  OUT     VOID   **DataPtr,
  IN      UINTN  DataSize,
  IN      UINTN  Function
  )
{
  EFI_SMM_COMMUNICATE_HEADER  *SmmCommunicateHeader;
  SPI_COMM_HEADER             *SpiCommHeader;

  //
  // The whole buffer size: SMM_COMMUNICATE_HEADER_SIZE + SPI_COMM_HEADER + DataSize.
  //
  SmmCommunicateHeader = AllocateZeroPool (DataSize + SMM_COMMUNICATE_HEADER_SIZE + SPI_COMM_HDR_SIZE);
  ASSERT (SmmCommunicateHeader != NULL);

  //
  // Prepare data buffer.
  //
  CopyGuid (&SmmCommunicateHeader->HeaderGuid, &gEfiDxeSmmSpiProtocolGuid);
  SmmCommunicateHeader->MessageLength = SPI_COMM_HDR_SIZE + DataSize;

  SpiCommHeader               = (SPI_COMM_HEADER *) SmmCommunicateHeader->Data;
  SpiCommHeader->Function     = Function;
  SpiCommHeader->ReturnStatus = EFI_NOT_FOUND;

  *CommunicateBuffer = SmmCommunicateHeader;
  if (DataPtr != NULL) {
    *DataPtr = SpiCommHeader->Data;
  }
}

/**
  Send the data in communicate buffer to SMI handler and get response.

  @param[in, out]  SmmCommunicateHeader    The communicate buffer.
  @param[in]       DataSize                The payload size.

  @return  ReturnStatus                    The return status from SMM communication.

**/
EFI_STATUS
SendCommunicateBuffer (
  IN OUT  EFI_SMM_COMMUNICATE_HEADER  *SmmCommunicateHeader,
  IN      UINTN                       DataSize
  )
{
  EFI_STATUS       Status;
  UINTN            CommSize;
  SPI_COMM_HEADER  *SpiCommHeader;

  CommSize = DataSize + SMM_COMMUNICATE_HEADER_SIZE + SPI_COMM_HDR_SIZE;
  Status   = mSmmCommunication->Communicate (
                                  mSmmCommunication, 
                                  SmmCommunicateHeader, 
                                  &CommSize
                                  );
  ASSERT_EFI_ERROR (Status);

  SpiCommHeader = (SPI_COMM_HEADER *) SmmCommunicateHeader->Data;
  return  SpiCommHeader->ReturnStatus;
}

/**
  Read data from the flash part.

  @param[in] FlashRegionType      The Flash Region type for flash cycle which is listed in the Descriptor.
  @param[in] Address              The Flash Linear Address must fall within a region for which BIOS has access permissions.
  @param[in] ByteCount            Number of bytes in the data portion of the SPI cycle.
  @param[out] Buffer              The Pointer to caller-allocated buffer containing the data received.
                                  It is the caller's responsibility to make sure Buffer is large enough for the total number of bytes read.

  @retval EFI_SUCCESS             Command succeed.
  @retval Other                   Some error occurs when executing this function.
**/
EFI_STATUS
EFIAPI
DxeSpiRead (
  IN     FLASH_REGION_TYPE  FlashRegionType,
  IN     UINT32             Address,
  IN     UINT32             ByteCount,
  OUT    UINT8              *Buffer
  )
{
  EFI_STATUS                  Status;
  UINTN                       PayloadSize;
  EFI_SMM_COMMUNICATE_HEADER  *SmmCommunicateHeader;
  SMM_SPI_READ_DATA           *SmmSpiReadData;

  //
  // Initialize the communicate buffer.
  //
  PayloadSize = sizeof(SMM_SPI_READ_DATA) + ByteCount;
  InitCommunicateBuffer ((VOID **)&SmmCommunicateHeader, (VOID **)&SmmSpiReadData, PayloadSize, FUNCTION_SPIFLASH_READ);
  if (SmmCommunicateHeader == NULL || SmmSpiReadData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  SmmSpiReadData->FlashRegionType = FlashRegionType;
  SmmSpiReadData->Address         = Address;
  SmmSpiReadData->ByteCount       = PayloadSize - sizeof(SMM_SPI_READ_DATA);
  SmmSpiReadData->Buffer          = (UINT8 *)(SmmSpiReadData + 1);

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (SmmCommunicateHeader, PayloadSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get data from SMM.
  //
  CopyMem (Buffer, SmmSpiReadData->Buffer, SmmSpiReadData->ByteCount);
  FreePool (SmmCommunicateHeader);

  return Status;
}

/**
  Write data to the flash part.

  @param[in] FlashRegionType      The Flash Region type for flash cycle which is listed in the Descriptor.
  @param[in] Address              The Flash Linear Address must fall within a region for which BIOS has access permissions.
  @param[in] ByteCount            Number of bytes in the data portion of the SPI cycle.
  @param[in] Buffer               Pointer to caller-allocated buffer containing the data sent during the SPI cycle.

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
EFI_STATUS
EFIAPI
DxeSpiWrite (
  IN     FLASH_REGION_TYPE  FlashRegionType,
  IN     UINT32             Address,
  IN     UINT32             ByteCount,
  IN     UINT8              *Buffer
  )
{
  EFI_STATUS                  Status;
  UINTN                       PayloadSize;
  EFI_SMM_COMMUNICATE_HEADER  *SmmCommunicateHeader;
  SMM_SPI_WRITE_DATA          *SmmSpiWriteData;

  //
  // Initialize the communicate buffer.
  //
  PayloadSize = sizeof(SMM_SPI_WRITE_DATA) + ByteCount;
  InitCommunicateBuffer ((VOID **)&SmmCommunicateHeader, (VOID **)&SmmSpiWriteData, PayloadSize, FUNCTION_SPIFLASH_WRITE);
  if (SmmCommunicateHeader == NULL || SmmSpiWriteData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  SmmSpiWriteData->FlashRegionType = FlashRegionType;
  SmmSpiWriteData->Address         = Address;
  SmmSpiWriteData->ByteCount       = PayloadSize - sizeof(SMM_SPI_WRITE_DATA);
  SmmSpiWriteData->Buffer          = (UINT8 *)(SmmSpiWriteData + 1);

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (SmmCommunicateHeader, PayloadSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Get data from SMM.
  //
  CopyMem (Buffer, SmmSpiWriteData->Buffer, SmmSpiWriteData->ByteCount);
  FreePool (SmmCommunicateHeader);

  return Status;
}

/**
  Entry Point for DXE driver.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
DxeSpiEntryPoint (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_SYSTEM_TABLE      *SystemTable
  )
{
  EFI_STATUS                      Status;
  EFI_HANDLE                      Handle;

  //
  // Locate SMM Communication Protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiSmmCommunicationProtocolGuid, 
                  NULL,
                  (VOID **) &mSmmCommunication
                  );
  ASSERT_EFI_ERROR (Status);

  // Install the Dxe Spi Protocol onto a new handle.
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiDxeSmmSpiProtocolGuid,
                  &mDxeSpiProtocol,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}