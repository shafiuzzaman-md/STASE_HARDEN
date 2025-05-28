#include "CharConverter.h"

NOTIFY_PROTOCOL_READY_PROTOCOL      mCharConverterReady;



EFI_STATUS
EFIAPI
IconvOpen (
    IN CONST CHAR8 *ToEncoding,
    IN CONST CHAR8 *FromEncoding,
    OUT ICONV_T    **CharDesc
)
{
    *CharDesc = AllocateZeroPool(sizeof(ICONV_T));
    if ((*CharDesc) == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }

    // Store encoding types
    (*CharDesc)->FromEncoding = AllocateZeroPool(AsciiStrSize(FromEncoding));
    if ((*CharDesc)->FromEncoding == NULL) {
        FreePool((*CharDesc));
        (*CharDesc) = NULL;
        return EFI_OUT_OF_RESOURCES;
    }
    AsciiStrCpyS((*CharDesc)->FromEncoding, AsciiStrSize(FromEncoding), FromEncoding);

    (*CharDesc)->ToEncoding = AllocateZeroPool(AsciiStrSize(ToEncoding));
    if ((*CharDesc)->ToEncoding == NULL) {
        FreePool((*CharDesc)->FromEncoding);
        FreePool((*CharDesc));
        (*CharDesc) = NULL;
        return EFI_OUT_OF_RESOURCES;
    }
    AsciiStrCpyS((*CharDesc)->ToEncoding, AsciiStrSize(ToEncoding), ToEncoding);

    return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
Iconv (
    IN ICONV_T    *CharDesc,
    IN CHAR8      *InputBuffer,
    IN INTN  InputSize,
    OUT CHAR8     **OutputBuffer,
    IN OUT INTN  *OutputSize
){
    if (AsciiStrCmp(CharDesc->FromEncoding, "UTF-8") == 0 &&
        AsciiStrCmp(CharDesc->ToEncoding, "UTF-16") == 0) {
        
        if (*OutputBuffer == NULL) {
            return EFI_OUT_OF_RESOURCES;
        }

        for (UINTN i = 0; i < InputSize; i++) {
            (*OutputBuffer)[i] = (CHAR16) InputBuffer[i]; 
            (*OutputSize)--;
        }

        (*OutputBuffer)[InputSize] = L'\0';
        return EFI_SUCCESS;

    } else if (AsciiStrCmp(CharDesc->FromEncoding, "UTF-16") == 0 &&
               AsciiStrCmp(CharDesc->ToEncoding, "UTF-8") == 0) {
        
        if (*OutputBuffer == NULL) {
            return EFI_OUT_OF_RESOURCES;
        }

        for (UINTN i = 0; i < InputSize / 2; i++) {
            CHAR16 *utf16_str = (CHAR16 *)InputBuffer;
            if (utf16_str[i] <= 0x7F) {
                (*OutputBuffer)[i] = (CHAR8)utf16_str[i];
            } else {
                (*OutputBuffer)[i] = '?'; 
            }
            (*OutputSize)--;
        }

        (*OutputBuffer)[InputSize / 2] = '\0';
        return EFI_SUCCESS;
    }
    else if (AsciiStrCmp(CharDesc->FromEncoding, "ISO-8859-1") == 0 &&
        AsciiStrCmp(CharDesc->ToEncoding, "UTF-16") == 0) {

        if (*OutputBuffer == NULL) {
            return EFI_OUT_OF_RESOURCES;
        }    
        for (UINTN i = 0; i < InputSize; i++) {
            (*OutputSize)--;
            if ((InputBuffer[i] & 0x80) == 0) {
                (*OutputBuffer)[i] = (CHAR16)InputBuffer[i];
            } else {

                if (i + 4 > InputSize) 
                {
                    (*OutputBuffer)[i++] = 0x1B;
                    (*OutputSize)--;
                    (*OutputBuffer)[i++] = '$';
                    (*OutputSize)--;
                    (*OutputBuffer)[i++] = '*';
                    (*OutputSize)--;
                    (*OutputBuffer)[i] = 'H';
                    break;
                }
                else
                {
                    (*OutputBuffer)[i] = 0xFD;
                }
            }
        }
            
        (*OutputBuffer)[InputSize + 3] = L'\0';
        (*OutputSize) -= 4; // Adjust for the added escape sequence
        return EFI_SUCCESS;
    }
    else if (AsciiStrCmp(CharDesc->FromEncoding, "UTF-8") == 0 &&
        AsciiStrCmp(CharDesc->ToEncoding, "ISO-8859-1") == 0) {

        if (*OutputBuffer == NULL) {
            return EFI_OUT_OF_RESOURCES;
        }

        UINTN OutIndex = 0;
        for (UINTN i = 0; i < InputSize; ) {
            UINT8 Char = InputBuffer[i];

            if (Char <= 0x7F) {
                (*OutputBuffer)[OutIndex++] = (CHAR8)Char;
                i++;
            }
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
            else if ((Char & 0xF0) == 0xE0 && i + 2 < InputSize) {
                // 3-byte UTF-8 (0x800 - 0xFFFF)
                UINT8 Char2 = InputBuffer[i + 1];
                UINT8 Char3 = InputBuffer[i + 2];

                if ((Char2 & 0xC0) == 0x80 && (Char3 & 0xC0) == 0x80) {
                    UINT16 UnicodeChar = ((Char & 0x0F) << 12) |
                                        ((Char2 & 0x3F) << 6) |
                                        (Char3 & 0x3F);

                    if (UnicodeChar <= 0xFF) {
                        // Valid ISO-8859-1 character
                        (*OutputBuffer)[OutIndex++] = (CHAR8)UnicodeChar;
                    }
                    i += 3;
                }
                else {
                    (*OutputBuffer)[OutIndex++] = Char;
                    i++;
                }
            }
            else {
                (*OutputBuffer)[OutIndex++] = Char; 
                i++;
            }
        }

        (*OutputSize) = (*OutputSize) - OutIndex;
        // Print the bytes of the output buffer to see what data overflowed
        // DEBUG((DEBUG_ERROR, "OutputBuffer Contents: "));
        // for (UINTN j = 0; j < OutIndex+3; j++) {
        //     DEBUG((DEBUG_ERROR, "%02X ", (UINT8)(*OutputBuffer)[j]));
        //     // print the addresses for the last 5 bytes
        //     if (j >= OutIndex - 5) {
        //         DEBUG((DEBUG_ERROR, "(%p) ", &(*OutputBuffer)[j]));
        //     }
        // }
        // DEBUG((DEBUG_ERROR, "\n"));

        return EFI_SUCCESS;
    }


    return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
StandardConvert(
    IN CHAR8 *Input,
    IN INTN *Size,
    IN HEAP_MANAGER_PROTOCOL *HeapManagerProtocol
) {
    ICONV_T *cd = NULL;
    EFI_STATUS Status = IconvOpen("ISO-8859-1", "UTF-8", &cd);
    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "IconvOpen Failed\n"));
        return Status;
    }
    CHUNK_T *Output = NULL;

    if (*Size <= BLOCK_080){
        HeapManagerProtocol->AllocateChunk(B_080, &Output);
    }
    else if (*Size <= BLOCK_100){
        HeapManagerProtocol->AllocateChunk(B_100, &Output);
    }
    else if (*Size <= BLOCK_200){
        HeapManagerProtocol->AllocateChunk(B_200, &Output);
    }
    else {
        DEBUG((DEBUG_ERROR, "...input is too large,UINTN giving up\n"));
        return EFI_INVALID_PARAMETER;
    }
    
    INTN OutputSize = *Size;
    CHAR8 *InputPtr = Input;
    CHAR8 *OutputPtr = (CHAR8 *)Output;
    Status = Iconv(cd, InputPtr, *Size, &OutputPtr, &OutputSize);
    Input = OutputPtr;
    *Size = OutputSize;
    if(EFI_ERROR(Status))
        return EFI_ABORTED;

    return Status;
}

EFI_STATUS
EFIAPI
Libxml2Convert(
    IN CHAR8 *Input,
    IN INTN *Size,
    IN HEAP_MANAGER_PROTOCOL *HeapManagerProtocol
) {
    ICONV_T *cd = NULL;
    EFI_STATUS Status = IconvOpen("ISO-8859-1", "UTF-8", &cd);
    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "IconvOpen Failed\n"));
        return Status;
    }
    CHUNK_T *Output = NULL;

    if (*Size*2 <= BLOCK_080){
        HeapManagerProtocol->AllocateChunk(B_080, &Output);
    }
    else if (*Size*2 <= BLOCK_100){
        HeapManagerProtocol->AllocateChunk(B_100, &Output);
    }
    else if (*Size*2 <= BLOCK_200){
        HeapManagerProtocol->AllocateChunk(B_200, &Output);
    }
    else {
        DEBUG((DEBUG_ERROR, "...input is too large, giving up\n"));
        return EFI_INVALID_PARAMETER;
    }
    if (EFI_ERROR(Status))
        return Status;
    INTN OutputSize = *Size*2;
    CHAR8 *InputPtr = Input;
    CHAR8 *OutputPtr = (CHAR8 *)Output;
    Status = Iconv(cd, InputPtr, *Size, &OutputPtr, &OutputSize);
    Input = OutputPtr;
    *Size = OutputSize; // Update the size to reflect the converted output
    if(EFI_ERROR(Status))
        return EFI_ABORTED;

    return Status;
}

EFI_STATUS
EFIAPI
PkexecConvert(
    IN CHAR8 *Input,
    IN INTN *Size,
    IN HEAP_MANAGER_PROTOCOL *HeapManagerProtocol
) {
    ICONV_T *cd = NULL;
    EFI_STATUS Status = IconvOpen("ISO-8859-1", "UTF-8", &cd);
    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "IconvOpen Failed\n"));
        return Status;
    }
    CHUNK_T *Output = NULL;
    
    if (*Size + NUL_TERMINATOR_LENGTH <= BLOCK_080){
        HeapManagerProtocol->AllocateChunk(B_080, &Output);
    }
    else if (*Size + NUL_TERMINATOR_LENGTH <= BLOCK_100){
        HeapManagerProtocol->AllocateChunk(B_100, &Output);
    }
    else if (*Size + NUL_TERMINATOR_LENGTH <= BLOCK_200){
        HeapManagerProtocol->AllocateChunk(B_200, &Output);
    }
    else {
        DEBUG((DEBUG_ERROR, "...input is too large, giving up\n"));
        return EFI_INVALID_PARAMETER;
    }
    if (EFI_ERROR(Status))
        return Status;
    INTN OutputSize = *Size + NUL_TERMINATOR_LENGTH;
    CHAR8 *InputPtr = Input;
    CHAR8 *OutputPtr = (CHAR8 *)Output;
    Status = Iconv(cd, InputPtr, *Size, &OutputPtr, &OutputSize);
    Input = OutputPtr;
    *Size = OutputSize; // Update the size to reflect the converted output
    if(EFI_ERROR(Status))
        return EFI_ABORTED;

    return Status;
}

EFI_STATUS
EFIAPI
CharConverterEntryPoint(
    IN EFI_HANDLE       ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status;
    EFI_HANDLE HandleProtoc = NULL;
    EFI_HANDLE HandleNotify = NULL;
    CHAR_CONVERTER_PROTOCOL *mCharConverter;
    UINTN      ProtocolSize = sizeof(CHAR_CONVERTER_PROTOCOL);
    
    //
    //
    // Allocate a memory pool for the protocol
    Status = gSmst->SmmAllocatePool(
                    EfiRuntimeServicesData,
                    ProtocolSize,
                    (VOID **)&mCharConverter
                    );
    if (EFI_ERROR (Status)) {
        return Status;
    }

    //
    //
    // Initialize the protocol instance
    mCharConverter->IconvOpen = IconvOpen;
    mCharConverter->Iconv = Iconv;
    mCharConverter->StandardConvert = StandardConvert;
    mCharConverter->Libxml2Convert = Libxml2Convert;
    mCharConverter->PkexecConvert = PkexecConvert;

    //
    //
    // Install the protocol
    Status = gSmst->SmmInstallProtocolInterface(
                        &HandleProtoc,
                        &gEfiCharConverterProtocolGuid,
                        EFI_NATIVE_INTERFACE,
                        mCharConverter
                        );
    if (EFI_ERROR (Status)) {
        return Status;
    }

    mCharConverterReady.ReadyFlags = ~(0x00);
    Status = gSmst->SmmInstallProtocolInterface(
                    &HandleNotify,
                    &gEfiCharConverterReadyProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mCharConverterReady
                    );
    if (EFI_ERROR (Status)) {
        return Status;
    }

    return Status;
}