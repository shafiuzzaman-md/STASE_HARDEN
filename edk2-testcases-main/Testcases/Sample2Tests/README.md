# Sample 2 Testcases

The Sample 2 testcases are intended to demonstrate the 3 WMI's similar to that in the **ICONV vulnerability** findings by [Ambionics](https://www.ambionics.io/blog/iconv-cve-2024-2961-p1). Based of the given [example](https://code.iti.illinois.edu/harden/harden-ta3-cromulence/phase2_samples/sample2), we tried to  showcase **three primary WMIs**. Each WMI has **preconditions, execution steps, and effects**:

| **WMI** | **Preconditions** | **Operations** | **Effects** |
|---------|------------------|---------------|-------------|
| **OOB Write** | input and output buffer size equal | Missing bounds check allows expansion to OOB write | Corrupts subsequent data (free list's next ptr) |
| **Write-What-Where** | Heap control to groom free/chunk lists | OOB Write overwrite next free list pointer LSB | Arbitrary write |
| **Control Flow Hijacking** | Writable function pointer | Overwrite pointer to redirect execution | Call attacker-controlled function |

We implemented our testcases based off the [basic](https://code.iti.illinois.edu/harden/harden-ta3-cromulence/phase2_samples/sample2/-/tree/main/1-basic?ref_type=heads) example given in the challenges.

## Overview

Similar to the basic example given, we provide an interactive way to experiment and trigger the WMIs, except we do this through SWSMI callouts. The driver `Sample2TestDriver` provides the SMI handler to process the "menu" and perform the appropriate operations. Our `HeapManager` driver is a SMM driver that provides a protocol to perform the heap operations described in the vulnerability. The final driver we implemented is `CharConverter`, which provides the protocol to perform the `iconv` operations and other basic conversions between charsets.

### `CharConverter`

We implemented the following protocol:
```
typedef struct _CHAR_CONVERTER_PROTOCOL {
    CHAR_CONVERTER_ICONV_OPEN        IconvOpen;
    CHAR_CONVERTER_ICONV             Iconv;
    CHAR_CONVERTER_STANDARD_CONVERT  StandardConvert;
    CHAR_CONVERTER_LIBXML2_CONVERT   Libxml2Convert;
    CHAR_CONVERTER_PKEXEC_CONVERT    PkexecConvert;
} CHAR_CONVERTER_PROTOCOL;
```
The `Iconv` implemention isn't all encompassing, but we implemented 4 dummy charset conversions:
* `UTF-8` to `UTF-16`
* `UTF-16` to `UTF-8`
* `ISO-8859-1` to `UTF-16`
* `UTF-8` to `ISO-8859-1`

The first two are implemented correctly (or we intended that to be the case) and the third conversion (`ISO-8859-1` to `UTF-16`) is another dummy conversion, but the fourth (`UTF-8` to `ISO-8859-1`) we included the missed check on the escape character that happened in the original WMI which will lead to the **OOB Write**.

This **OOB Write** can be seen in the following snippet of code, which is one of the sub `if` statements responsible for converting between the charsets. Specifically, the two byte characters:
```
else if ((Char & 0xE0) == 0xC0 && i + 1 < InputSize) {
    UINT8 Char2 = InputBuffer[i + 1];

    if ((Char2 & 0xC0) == 0x80) {
        UINT16 UnicodeChar = ((Char & 0x1F) << 6) | (Char2 & 0x3F);
        if (UnicodeChar <= 0xFF) {
            (*OutputBuffer)[OutIndex++] = (CHAR8)UnicodeChar;
        }
        else {
            // Buffer overflow bug here! No bound check.
            (*OutputBuffer)[OutIndex++] = 0x1B; // ESC
            (*OutputBuffer)[OutIndex++] = '$';
            (*OutputBuffer)[OutIndex++] = '*';
            (*OutputBuffer)[OutIndex++] = 'H';
            break;
        }
        i += 2;
    }
    else {
        // Invalid UTF-8 sequence, trigger ESC sequence bug
        (*OutputBuffer)[OutIndex++] = Char;
        i++;
    }
}
```

It is not identical in terms of the bug location, but we did implement the same side affects. Meaning we will get a 1-2 byte overwrite if there is a two byte character that meets the wierd conditions above and the overwrite will be known bytes.

### `HeapManager`

Here we implemented the following heap manager protocol:
```
typedef struct _HEAP_MANAGER_PROTOCOL {
    HEAP_MANAGER_GET_BLOCK_SIZE            GetBlockSize;
    HEAP_MANAGER_ALLOCATE_CHUNK            AllocateChunk;
    HEAP_MANAGER_FREE_CHUNK                FreeChunk;
    HEAP_MANAGER_FREE_ALLOC_LIST           FreeAllocList;
    HEAP_MANAGER_DISPLAY_FREE_LIST         DisplayFreeList;
    HEAP_MANAGER_DISPLAY_HEAP_METADATA     DisplayHeapMetadata;
    HEAP_MANAGER_INITIALIZE_PAGE           InitializePage;
    HEAP_MANAGER_INITIALIZE_HEAP_METADATA  InitializeHeapMetadata;
    HEAP_MANAGER_INITIALIZE_HEAP           InitializeHeap;
} HEAP_MANAGER_PROTOCOL;
```

This protocol will provide the preconditions described in the table above for the **Write-What-Where** WMI, so by utilizing this protocols functionality along with the **OOB Write** provided from the `CharConverter` driver we can obtain arbtrary write.

### `Sample2TestsDriver`

This driver is a SWSMI callout that takes in data through the `CommBuffer` with the following structure:
```
typedef struct {
    EFI_GUID          Sample2Guid;
    enum OP_TYPE      Operation;
    enum CONVERT_TYPE Conversion;
    UINT8             Data[DATA_LENGTH];
} SAMPLE2_TESTS_DRIVER_COMM_BUFFER;
```
The `Sample2Guid` isn't needed but was left to make it easier to add future funtionality to this testcase. The `OP_TYPE` defineds the intended operation or "menu" interaction:
```
enum OP_TYPE{
    CONVERT,
    FREE_LIST,
    DISPLAY_LIST,
    DISPLAY_METADATA,
    MSG
};
```
`MSG` will print the contents of the function pointer, so this will be used similarly to the basic example to eventually print "pwned" once the function pointer is overwritten. The rest of the menu options are self explanitory. Finally, The `CONVERT_TYPE` is only used when the `CONVERT` menu option is used, and it has the following options:
```
enum CONVERT_TYPE {
    STANDARD,
    LIBXML2,
    PKEXEC
};
```
It is important to know that the heap and heap metadata are both initialized when the driver is initialized so it will not re-initialize for each interaction. Therefore, it will achieve the same result as the example given where the heap is constant throughout the entire interaction.
