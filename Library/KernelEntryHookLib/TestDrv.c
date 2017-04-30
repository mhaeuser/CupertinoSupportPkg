#include <Uefi.h>
// KernelEntryHook
VOID
KernelEntryHook (
  IN OUT UINT32  KernelEntry
);
EFI_STATUS
EFIAPI
Start (
  EFI_HANDLE Handle,
  EFI_SYSTEM_TABLE *ST
)
{
  KernelEntryHook (0);
  return EFI_SUCCESS;
}
