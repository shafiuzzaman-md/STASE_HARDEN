#include "ECH.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "klee/klee.h"

extern EFI_SMM_SYSTEM_TABLE2 MockSmst;
extern EFI_SMM_SYSTEM_TABLE2 *gSmst;

#include "instrumented_code/HardwareHandler.c"

HARDWARE_MANAGER_LICENSE_PROTOCOL   *mHardwareManagerLicenseProtocol  = NULL;
NOTIFY_PROTOCOL_READY_PROTOCOL      mHardwareHandlerReady;

/* Stub: SmmLocateProtocol */
EFI_STATUS
EFIAPI
Stub_SmmLocateProtocol(
  IN EFI_GUID *Protocol,
  IN VOID     *Registration,
  OUT VOID    **Interface
)
{
  if (!Interface) {
    return EFI_INVALID_PARAMETER;
  }
  static char dummyInterface[64];
  memset(dummyInterface, 0, sizeof(dummyInterface));
  *Interface = dummyInterface;

  return EFI_SUCCESS;
}

/* Stub: SmmAllocatePool */
EFI_STATUS
EFIAPI
Stub_SmmAllocatePool(
  IN EFI_MEMORY_TYPE PoolType,
  IN UINTN Size,
  OUT VOID **Buffer
)
{
  if (!Buffer) {
    return EFI_INVALID_PARAMETER;
  }
  *Buffer = malloc(Size);
  if (!*Buffer) {
    return EFI_OUT_OF_RESOURCES;
  }
  memset(*Buffer, 0, Size);
  return EFI_SUCCESS;
}

/* Stub: SmmInstallProtocolInterface */
EFI_STATUS
EFIAPI
Stub_SmmInstallProtocolInterface(
  IN OUT EFI_HANDLE *Handle,
  IN EFI_GUID       *Protocol,
  IN EFI_INTERFACE_TYPE InterfaceType,
  IN VOID           *Interface
)
{
  printf("[stub] SmmInstallProtocolInterface called (Protocol=%p, Interface=%p)\n",
         (void*)Protocol, Interface);
  if (Handle) {
    *Handle = (EFI_HANDLE)Protocol; // Some dummy handle
  }
  return EFI_SUCCESS;
}

/* main() - sets up the stubs and declares ImageHandle, SystemTable as symbolic */
int main(void) {
  printf("=== Running KLEE driver for HardwareHandler tests ===\n");

  /* Initialize the mock function pointers. */
  MockSmst.SmmLocateProtocol            = Stub_SmmLocateProtocol;
  MockSmst.SmmAllocatePool             = Stub_SmmAllocatePool;
  MockSmst.SmmInstallProtocolInterface = Stub_SmmInstallProtocolInterface;

  EFI_HANDLE ImageHandle;
  EFI_SYSTEM_TABLE *SystemTable;

  klee_make_symbolic(&ImageHandle, sizeof(ImageHandle), "ImageHandle");
  klee_make_symbolic(&SystemTable, sizeof(SystemTable), "SystemTable");

  EFI_STATUS status = HardwareHandlerEntryPoint(ImageHandle, SystemTable);
  printf("HardwareHandlerEntryPoint => EFI_STATUS=%u\n", (unsigned)status);

  return 0;
}
