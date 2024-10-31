/** @file

  The dxe header file.

**/

#ifndef _DXE_SPI_H_
#define _DXE_SPI_H_

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SmmCommunication.h>

#include "SmmSpiCommon.h"

typedef struct _DXE_SPI_PROTOCOL  DXE_SPI_PROTOCOL;

EFI_STATUS
EFIAPI
DxeSpiRead (
  IN     FLASH_REGION_TYPE  FlashRegionType,
  IN     UINT32             Address,
  IN     UINT32             ByteCount,
  OUT    UINT8              *Buffer
  );

EFI_STATUS
EFIAPI
DxeSpiWrite (
  IN     FLASH_REGION_TYPE  FlashRegionType,
  IN     UINT32             Address,
  IN     UINT32             ByteCount,
  OUT    UINT8              *Buffer
  );
#endif