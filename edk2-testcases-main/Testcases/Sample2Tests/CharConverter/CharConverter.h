#ifndef __CHAR_CONVERTER_H__
#define __CHAR_CONVERTER_H__

#include <Library/MmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/SmmMemLib.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/SmmMemoryAttribute.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/IoLib.h>
#include <PiSmm.h>
#include <Uefi.h>
#include <Protocol/SmmBase2.h>

#include "../HeapManager/HeapManager.h"

extern EFI_GUID gEfiCharConverterProtocolGuid;
extern EFI_GUID gEfiCharConverterReadyProtocolGuid;

typedef struct {
    CHAR8 *FromEncoding;
    CHAR8 *ToEncoding;
} ICONV_T;

#define ISO_8859_1_set 0x10
#define UTF_16_set     0x20
#define ESC            0x1B
#define BLOCK_080 0x80
#define BLOCK_100 0x100
#define BLOCK_200 0x200
#define NUL_TERMINATOR_LENGTH 4

typedef
EFI_STATUS
(EFIAPI *CHAR_CONVERTER_ICONV_OPEN)(
    IN CONST CHAR8 *ToEncoding,
    IN CONST CHAR8 *FromEncoding,
    OUT ICONV_T    **CharDesc 
);

typedef
EFI_STATUS
(EFIAPI *CHAR_CONVERTER_ICONV)(
    IN ICONV_T    *CharDesc,
    IN CHAR8      *InputBuffer,
    IN OUT INTN  InputSize,
    OUT CHAR8     **OutputBuffer,
    IN OUT INTN  *OutputSize
);

typedef
EFI_STATUS
(EFIAPI *CHAR_CONVERTER_STANDARD_CONVERT)(
    IN CHAR8 *Input,
    IN INTN *Size,
    IN HEAP_MANAGER_PROTOCOL *HeapManagerProtocol
);

typedef
EFI_STATUS
(EFIAPI *CHAR_CONVERTER_LIBXML2_CONVERT)(
    IN CHAR8 *Input,
    IN INTN *Size,
    IN HEAP_MANAGER_PROTOCOL *HeapManagerProtocol
);

typedef
EFI_STATUS
(EFIAPI *CHAR_CONVERTER_PKEXEC_CONVERT)(
    IN CHAR8 *Input,
    IN INTN *Size,
    IN HEAP_MANAGER_PROTOCOL *HeapManagerProtocol
);

typedef struct _CHAR_CONVERTER_PROTOCOL {
    CHAR_CONVERTER_ICONV_OPEN        IconvOpen;
    CHAR_CONVERTER_ICONV             Iconv;
    CHAR_CONVERTER_STANDARD_CONVERT  StandardConvert;
    CHAR_CONVERTER_LIBXML2_CONVERT   Libxml2Convert;
    CHAR_CONVERTER_PKEXEC_CONVERT    PkexecConvert;
} CHAR_CONVERTER_PROTOCOL;

#endif // __CHAR_CONVERTER_H__