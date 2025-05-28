#ifndef _SAMPLE2_TESTS_DRIVER_H_
#define _SAMPLE2_TESTS_DRIVER_H_

#include <PiSmm.h>
#include <Uefi.h>
#include <Protocol/SmmBase2.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include "../HeapManager/HeapManager.h"
#include "../CharConverter/CharConverter.h"

extern EFI_GUID gEfiSample2TestsDriverCommGuid;

enum OP_TYPE{
    CONVERT,
    FREE_LIST,
    DISPLAY_LIST,
    DISPLAY_METADATA,
    MSG
};

enum CONVERT_TYPE {
    STANDARD,
    LIBXML2,
    PKEXEC
};

#define DATA_LENGTH 1024
#define MAX_CHAR_LENGTH 512

typedef struct {
    INTN Size;
    CHAR8 Data[MAX_CHAR_LENGTH];
} CHAR_CONVERTER_DATA;

typedef struct {
    EFI_GUID          Sample2Guid;
    enum OP_TYPE      Operation;
    enum CONVERT_TYPE Conversion;
    UINT8             Data[DATA_LENGTH];
} SAMPLE2_TESTS_DRIVER_COMM_BUFFER;

#endif // _SAMPLE2_TESTS_DRIVER_H_
