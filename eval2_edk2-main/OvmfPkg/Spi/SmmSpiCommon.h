/** @file

  The internal header file includes the common header files, defines
  internal structure and functions used by Dxe/Smm Spi drivers.

**/

#ifndef _SMM_SPI_COMMON_H_
#define _SMM_SPI_COMMON_H_

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SpiFlashLib/SpiCommon.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/SmmMemLib.h>
#include <Guid/EventGroup.h>

#define FUNCTION_SPIFLASH_READ                  1
#define FUNCTION_SPIFLASH_WRITE                 2

typedef struct {
  UINTN       Function;
  EFI_STATUS  ReturnStatus;
  UINT8       Data[1];
} SPI_COMM_HEADER;

typedef struct {
  FLASH_REGION_TYPE  FlashRegionType;
  UINT32             Address;
  UINT32             ByteCount;
  UINT8              *Buffer;
} SMM_SPI_READ_DATA;

typedef struct {
  FLASH_REGION_TYPE  FlashRegionType;
  UINT32             Address;
  UINT32             ByteCount;
  UINT8              *Buffer;
} SMM_SPI_WRITE_DATA;

//
// Size of SMM communicate header, without including the payload.
//
#define SMM_COMMUNICATE_HEADER_SIZE  (OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data))

//
// Size of comms header, without data.
//
#define SPI_COMM_HDR_SIZE  (OFFSET_OF (SPI_COMM_HEADER, Data))

//
// Size of read header, without buffer.
//
#define SPI_READ_HDR_SIZE  (OFFSET_OF (SMM_SPI_READ_DATA, Buffer))
//
// Size of write header, without buffer.
//
#define SPI_WRITE_HDR_SIZE  (OFFSET_OF (SMM_SPI_WRITE_DATA, Buffer))


/**
  Read data from the flash part.

  @param[in]  FlashRegionType    The Flash Region type for flash cycle which is listed in the Descriptor.
  @param[in]  Address            The Flash Linear Address must fall within a region for which BIOS has access permissions.
  @param[in]  ByteCount          Number of bytes in the data portion of the SPI cycle.
  @param[out] Buffer             The Pointer to caller-allocated buffer containing the data received.
                                 It is the caller's responsibility to make sure Buffer is large enough for the total number of bytes read.

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
typedef
EFI_STATUS
(EFIAPI *DXE_SPI_READ) (
  IN     FLASH_REGION_TYPE  FlashRegionType,
  IN     UINT32             Address,
  IN     UINT32             ByteCount,
  OUT    UINT8              *Buffer
  );

/**
  Write data to the flash part.

  @param[in] FlashRegionType    The Flash Region type for flash cycle which is listed in the Descriptor.
  @param[in] Address            The Flash Linear Address must fall within a region for which BIOS has access permissions.
  @param[in] ByteCount          Number of bytes in the data portion of the SPI cycle.
  @param[in] Buffer             Pointer to caller-allocated buffer containing the data sent during the SPI cycle.

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
typedef
EFI_STATUS
(EFIAPI *DXE_SPI_WRITE) (
  IN     FLASH_REGION_TYPE  FlashRegionType,
  IN     UINT32             Address,
  IN     UINT32             ByteCount,
  IN     UINT8              *Buffer
  );

struct _DXE_SPI_PROTOCOL {
  DXE_SPI_READ   SpiFlashRead;
  DXE_SPI_WRITE  SpiFlashWrite;
};

#endif
