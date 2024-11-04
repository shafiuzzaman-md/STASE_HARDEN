#ifndef ECH_H
#define ECH_H

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "klee/klee.h"

// Header files for local communication
#include "../eval2_edk2-main/MdePkg/Include/Uefi/UefiBaseType.h"
#include "../eval2_edk2-main/MdePkg/Include/Uefi/UefiMultiPhase.h"
#include "../eval2_edk2-main/BaseTools/Source/C/Include/Common/PiFirmwareVolume.h"
#include "../eval2_edk2-main/MdeModulePkg/Include/Protocol/FaultTolerantWrite.h"
#include "../eval2_edk2-main/MdePkg/Include/Uefi/UefiSpec.h"
#include "../eval2_edk2-main/MdePkg/Include/Protocol/DriverBinding.h"
#include "../eval2_edk2-main/MdePkg/Include/Protocol/ComponentName.h"
#include "../eval2_edk2-main/MdePkg/Include/Protocol/ComponentName2.h"
#include "../eval2_edk2-main/MdePkg/Include/IndustryStandard/Pci22.h"
#include "../eval2_edk2-main/MdePkg/Include/Library/BaseLib.h"
#include "../eval2_edk2-main/BaseTools/Source/C/Include/Protocol/GraphicsOutput.h"
#include "../eval2_edk2-main/MdeModulePkg/Bus/Pci/PciBusDxe/PciBus.h"
#include "../eval2_edk2-main/MdePkg/Library/SmmMemLib/SmmMemLib.c"
#include "../eval2_edk2-main/MdePkg/Library/BaseMemoryLib/CopyMemWrapper.c"
#include "../eval2_edk2-main/MdePkg/Library/BaseMemoryLib/CopyMem.c"
#include "../eval2_edk2-main/MdePkg/Library/BaseLib/Math64.c"
#include "../eval2_edk2-main/MdePkg/Library/BaseLib/DivU64x32.c"
#include "../eval2_edk2-main/StandaloneMmPkg/Library/StandaloneMmMemLib/StandaloneMmMemLib.c"
#include "../eval2_edk2-main/MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxDxeLib.c"
#include "../eval2_edk2-main/MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeCache.c"
#include "../eval2_edk2-main/MdeModulePkg/Universal/Variable/RuntimeDxe/TcgMorLockSmm.c"
#include "../eval2_edk2-main/MdeModulePkg/Include/Protocol/SmmVariable.h"

// typedef struct {
//   UINT16    Limit;
//   UINTN     Base;
// } IA32_DESCRIPTOR;

// Function to generate a symbolic value
int randomBoolean() {
    _Bool a;
    klee_make_symbolic(&a, sizeof(a), "a");
    printf("Bool: %d", a);
    return a; // Returns either 0 (false) or 1 (true)
}
#define PcdSmmLegacyPropertyMask (randomBoolean())
#define _PCD_GET_MODE_32_PcdSmmLegacyPropertyMask (randomBoolean())
#define _PCD_GET_MODE_BOOL_PcdUseAccessRWVariable (randomBoolean())

//System table pointers
EFI_MM_SYSTEM_TABLE  *gMmst = NULL; //MM System Table 
//EFI_SMM_SYSTEM_TABLE2  *gSmst = NULL; //System Management System Table
EFI_BOOT_SERVICES  *gBS         = NULL; //EFI Boot Services Table
EFI_DXE_SERVICES  *gDS = NULL; //DXE Services Table
//EFI_RUNTIME_SERVICES  *gRT = NULL;
EFI_SYSTEM_TABLE   *gST         = NULL;


// Define a mock SMST
EFI_SMM_SYSTEM_TABLE2  MockSmst;

//Protocol GUIDs
EFI_GUID gEfiSmmLegacyCommGuid, gEdkiiSmmMemoryAttributeProtocolGuid, gEfiSmmLegacyProtocolGuid, gEfiSmmLegacyVariableGuid, gEfiSmmFaultTolerantWriteProtocolGuid, gEfiSmmFirmwareVolumeBlockProtocolGuid, gEfiSmmVariableProtocolGuid,gEdkiiSmmVarCheckProtocolGuid,gEfiMmEndOfDxeProtocolGuid, gEfiSmmLockBoxCommunicationGuid,gEfiSmmReadyToLockProtocolGuid,gEfiLockBoxProtocolGuid,gEfiMemoryAttributesTableGuid,gEfiSmmAccess2ProtocolGuid,gEfiSmmEndOfDxeProtocolGuid,gEfiSmmSwapAddressRangeProtocolGuid,gEfiTcg2ProtocolGuid,gEfiTcgProtocolGuid,gSmmVariableWriteGuid,gEfiQueryVariableInfoCommGuid, gEfiQueryVariableInfoProtocolGuid,gEfiCallerIdGuid,gEdkiiMemoryProfileGuid,gEfiPeiSmmCommunicationPpiGuid,gEfiSmmSwDispatch2ProtocolGuid, gEfiMemoryOverwriteControlDataGuid, gEfiTpmDeviceInstanceTpm20DtpmGuid, gTpmNvsMmGuid, gEfiMmReadyToLockProtocolGuid,
gEfiEventDxeDispatchGuid, gEfiDxeSmmReadyToLockProtocolGuid, gEfiEventLegacyBootGuid, gEfiEventExitBootServicesGuid,
gEfiEventReadyToBootGuid, gEfiEndOfDxeEventGroupGuid, gEdkiiS3SmmInitDoneGuid, gEdkiiEndOfS3ResumeGuid, gEdkiiSmmLegacyBootProtocolGuid,
gEdkiiSmmExitBootServicesProtocolGuid,gEdkiiSmmReadyToBootProtocolGuid,gEfiSmmCpuIo2ProtocolGuid,gEfiSmmSxDispatch2ProtocolGuid,
gEfiLoadedImageProtocolGuid,gEfiFirmwareVolume2ProtocolGuid, gEfiSecurity2ArchProtocolGuid, gEfiSecurityArchProtocolGuid,
gEfiDevicePathProtocolGuid,gAprioriGuid,gEdkiiSmmMemoryProfileGuid, gEfiSmmCommunicationProtocolGuid, gEfiSmmCommunicationProtocolGuid,
gEfiSmmCommunicationProtocolGuid,gEdkiiPiSmmCommunicationRegionTableGuid,gEfiMemoryOverwriteRequestControlLockGuid,gEfiGlobalVariableGuid,gEfiHardwareErrorVariableGuid,
gEfiSmmVariableProtocolGuid,gEfiRWVariableProtocolGuid,gEfiRWVariableCommGuid,gEfiMmAccessProtocolGuid, gEfiAccessRWVarProtocolGuid, gZeroGuid;


//Global variables
EFI_PHYSICAL_ADDRESS  gLoadModuleAtFixAddressSmramBase;
EFI_LOADED_IMAGE_PROTOCOL  *mSmmCoreLoadedImage;
LIST_ENTRY  mSmmMemoryMap;
UINTN                 mFullSmramRangeCount;
EFI_SMRAM_DESCRIPTOR  *mFullSmramRanges;
VARIABLE_INFO_ENTRY  *gVariableInfo = NULL;// The memory entry used for variable statistics data.
VAR_CHECK_REQUEST_SOURCE  mRequestSource = VarCheckFromUntrusted; //categorizing variable check requests as coming from either trusted or untrusted sources
// SMM IPL global variables
//BOOLEAN                    mEndOfDxe  = FALSE;
VARIABLE_MODULE_GLOBAL  *mVariableModuleGlobal;
VARIABLE_STORE_HEADER  *mNvVariableCache = NULL;
EFI_FIRMWARE_VOLUME_HEADER  *mNvFvHeaderCache = NULL;
EFI_IPv4_ADDRESS  mZeroIp4Addr;

int SmmStackArrayBase;




// Global definitions or configurations used across your project
//#define BUFFER_SIZE 256  // Common buffer size for vulnerability checks
//#define MAX_COMMAND_LENGTH 100  // Maximum length of command inputs

// Setup or initialize KLEE symbolic variables or other configurations
void stase_init_env();

#endif // ECH_H