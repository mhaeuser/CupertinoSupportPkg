#ifndef FIRMWARE_FIXES_H_
#define FIRMWARE_FIXES_H_

// mStartImage
extern EFI_IMAGE_START mStartImage;

// mAllocatePages
extern EFI_ALLOCATE_PAGES mAllocatePages;

// mGetMemoryMap
extern EFI_GET_MEMORY_MAP mGetMemoryMap;

// mExitBootServices
extern EFI_EXIT_BOOT_SERVICES mExitBootServices;

// mHandleProtocol
extern EFI_HANDLE_PROTOCOL mHandleProtocol;

#if !defined (RELOC_BLOCK)

// mSetVirtualAddressMap
extern EFI_SET_VIRTUAL_ADDRESS_MAP mSetVirtualAddressMap;

// mHibernateImageAddress
/// location of memory allocated by boot.efi for hibernate image
extern EFI_PHYSICAL_ADDRESS mHibernateImageAddress;

#endif

// FixMemoryMap
VOID
FixMemoryMap (
  IN UINTN                  MemoryMapSize,
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                  DescriptorSize,
  IN UINT32                 DescriptorVersion
  );

// KernelEntryPatchJump
/** Patches kernel entry point with jump to JumpFromKernel (AsmFuncsX64). This will then call KernelEntryPatchJumpBack.
**/
EFI_STATUS
KernelEntryPatchJump (
  IN OUT UINT32  KernelEntry
  );

// ShrinkMemoryMap
VOID
ShrinkMemoryMap (
  IN UINTN                  *MemoryMapSize,
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                  DescriptorSize
  );

#endif // FIRMWARE_FIXES_H_
