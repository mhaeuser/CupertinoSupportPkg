/** @file
  Copyright (C) 2012 - 2014 Damir Mazar.  All rights reserved.<BR>
  Portions Copyright (C) 2015 - 2016 CupertinoNet.  All rights reserved.<BR>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in
  the documentation and/or other materials provided with the
  distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
**/

#include <Uefi.h>

#include <Guid/AppleNvram.h>

#include <Library/EfiVariableLib.h>
#include <Library/AppleDeviceTreeLib.h>

#include "AsmFuncs.h"
#include "FirmwareFixes.h"

// mRelocationBase
/// relocation base address
EFI_PHYSICAL_ADDRESS mRelocationBase = 0;

// mNoRelocationBlockPages
/// relocation block size in pages
UINTN mNoRelocationBlockPages = 0;

// GetNoRelocationBlockPages
/** Calculate the size of relocation block.
**/
UINTN
GetNoRelocationBlockPages (
  VOID
  )
{
  EFI_STATUS Status;

  UINTN      NoRtPages;
  UINTN      Size;

  Size = 0;

  // Sum pages needed for RT and MMIO areas
  Status = GetNoRuntimePages (&NoRtPages);

  if (!EFI_ERROR (Status)) {
    Size = (KERNEL_BLOCK_NO_RT_SIZE_PAGES + NoRtPages);
  }

  return Size;
}

// AllocateRelocationBlock
/// Allocate free block on top of mem for kernel image relocation (will be returned to boot.efi for kernel boot image).
EFI_STATUS
AllocateRelocationBlock (
  VOID
  )
{
  EFI_STATUS           Status;

  EFI_PHYSICAL_ADDRESS Address;

  mNoRelocationBlockPages = 0;

  // calculate the needed size for reloc block
  GetNoRelocationBlockPages (&mNoRelocationBlockPages);

  Address         = 0x100000000;  // max address
  Status          = AllocatePagesFromTop (EfiBootServicesData, mNoRelocationBlockPages, &Address);

  if (!EFI_ERROR (Status)) {
    mRelocationBase = Address;

    // set reloc addr in runtime vars for boot manager
    SetVariable (
      L"AptioFix-RelocBase",
      &gAppleBootVariableGuid,
      (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS),
      sizeof (mRelocationBase),
      (VOID *)&mRelocationBase
      );
  }

  return Status;
}

// FreeRelocationBlock
/// Releases relocation block.
EFI_STATUS
FreeRelocationBlock (
  VOID
  )
{
  EFI_STATUS Status;

  Status                  = FreePages (mRelocationBase, mNoRelocationBlockPages);
  mRelocationBase         = 0;
  mNoRelocationBlockPages = 0;

  return Status;
}

// FixBootingWithRelocBlock
/// Fixes stuff when booting with relocation block. Called when boot.efi jumps to kernel.
UINTN
FixBootingWithRelocBlock (
  IN UINTN  BootArgs
  ) // called as entry point
{
  BOOT_ARGS_PTR    BootArgsPtr;
  CLOVER_BOOT_ARGS *CloverBootArgs;
  UINTN            MemoryMapSize;

  BootArgsPtr.Version1 = (BOOT_ARGS_V1 *)BootArgs;
  CloverBootArgs       = GetBootArgs (BootArgsPtr);
  MemoryMapSize        = (UINTN)*CloverBootArgs->MemMapSize;

  ShrinkMemoryMap (
    &MemoryMapSize,
    (EFI_MEMORY_DESCRIPTOR *)(UINTN)*CloverBootArgs->MemMap,
    *CloverBootArgs->MemMapDescriptorSize,
    *CloverBootArgs->MemMapDescriptorVersion
    );

  *CloverBootArgs->MemMapSize = (UINT32)MemoryMapSize;

  FixRuntimeServices (CloverBootArgs);
  FixDeviceTree (CloverBootArgs);
  FixBootArgs (CloverBootArgs, mRelocationBase);

  BootArgs -= (UINTN)mRelocationBase;

  // set vars for copying kernel
  // note: *CloverBootArgs->KernelAddress is fixed in FixBootArgs() and points to real KernelAddress
  gAsmKernelImageStartReloc = (*CloverBootArgs->KernelAddress + (UINT32)mRelocationBase);
  gAsmKernelImageStart      = *CloverBootArgs->KernelAddress;
  gAsmKernelImageSize       = *CloverBootArgs->KernelSize;

  return BootArgs;
}

// FixDeviceTree
/** DevTree contains /chosen/memory-map with properties with 8 byte values
  (DTMemMapEntry: UINT32 Address, UINT32 Length):
  "name" = this is exception - not DTMemMapEntry
  "BootCLUT" = 8bit boot time colour lookup table
  "Pict-FailedBoot" = picture shown if booting fails
  "RAMDisk" = ramdisk
  "Driver-<hex addr of BooterKextFileInfo>" = Kext, UINT32 Address points to BooterKextFileInfo
  "DriversPackage-..." = MKext, UINT32 Address points to mkext_header (libkern/libkern/mkext.h), UINT32 length

  We are fixing here DTMemMapEntry.Address for all those entries.
  Plus, for every loaded kext, Address points to BooterKextFileInfo,
  and we are fixing it's pointers also.
**/
VOID
FixDeviceTree (
  IN CLOVER_BOOT_ARGS  *BootArgs
  ) // called as entry point
{
  DEVICE_TREE_ENTRY                    MemMap;
  OPAQUE_DEVICE_TREE_PROPERTY_ITERATOR OPropIter;
  DEVICE_TREE_PROPERTY_ITERATOR        PropIter;
  CHAR8                                *PropName;
  DEVICE_TREE_MEMORY_MAP_ENTRY         *PropValue;
  BOOTER_KEXT_FILE_INFO                *KextInfo;

  DeviceTreeInit ((DEVICE_TREE_ENTRY)(UINTN)*BootArgs->DeviceTreePtr);

  if (DeviceTreeLookupEntry (NULL, "/chosen/memory-map", &MemMap) == kSuccess) {
    PropIter = &OPropIter;

    if (DeviceTreeCreatePropertyIteratorNoAlloc (MemMap, PropIter) == kSuccess) {
      while (DeviceTreeIterateProperties (PropIter, &PropName) == kSuccess) {
        // all /chosen/memory-map props have DTMemMapEntry (address, length)
        // values. we need to correct the address

        // basic check that value is 2 * UINT32
        if (PropIter->CurrentProperty->Length == (2 * sizeof (UINT32))) {
          // get value (Address and Length)
          PropValue = (DEVICE_TREE_MEMORY_MAP_ENTRY *)(PropIter->CurrentProperty + 1);

          // second check - Address is in our reloc block
          // (note: *CloverBootArgs->KernelAddress is not fixed yet and points to reloc block)
          if ((PropValue->Address >= *BootArgs->KernelAddress) && (PropValue->Address <= (*BootArgs->KernelAddress + *BootArgs->KernelSize))) {
            // check if this is Kext entry
            if (AsciiStrnCmp (PropName, BOOTER_KEXT_PREFIX, AsciiStrLen (BOOTER_KEXT_PREFIX)) == 0) {
              // yes - fix kext pointers

              KextInfo                             = (BOOTER_KEXT_FILE_INFO *)(UINTN)PropValue->Address;
              KextInfo->InfoDictPhysicalAddress   -= (UINT32)mRelocationBase;
              KextInfo->ExecutablePhysicalAddress -= (UINT32)mRelocationBase;
              KextInfo->BundlePathPhysicalAddress -= (UINT32)mRelocationBase;
            }

            // fix address in mem map entry
            PropValue->Address -= (UINT32)mRelocationBase;
          }
        }
      }
    }
  }
}

// AssignVirtualAddressesToMemMap
/// Assignes OSX virtual addresses to runtime areas in memory map.
VOID
AssignVirtualAddressesToMemMap (
  IN UINTN                  MemMapSize,
  IN UINTN                  DescriptorSize,
  IN EFI_MEMORY_DESCRIPTOR  *MemMap,
  IN EFI_PHYSICAL_ADDRESS   KernelRTAddress
  )
{
  UINTN Index;

  for (Index = 0; Index < (MemMapSize / DescriptorSize); ++Index) {
    // assign virtual addresses to all EFI_MEMORY_RUNTIME marked pages (including MMIO)
    if (BIT_SET (MemMap->Attribute, EFI_MEMORY_RUNTIME)
      && ((MemMap->Type == EfiRuntimeServicesCode)
        || (MemMap->Type == EfiRuntimeServicesData)
        || (MemMap->Type == EfiMemoryMappedIO)
        || (MemMap->Type == EfiMemoryMappedIOPortSpace))) {
      // for RT/MMIO block - assign from kernel block
      MemMap->VirtualStart = (EFI_VIRTUAL_ADDRESS)(KernelRTAddress + 0xffffff8000000000);
      KernelRTAddress     += (EFI_PHYSICAL_ADDRESS)EFI_PAGES_TO_SIZE (MemMap->NumberOfPages);
    }

    // else: runtime flag, but not RT and not MMIO - what now?

    MemMap = NEXT_MEMORY_DESCRIPTOR (MemMap, DescriptorSize);
  }
}

// FixRuntimeServices
/// Fixes RT vars in bootArgs, virtualizes and defragments RT blocks.
VOID
FixRuntimeServices (
  IN CLOVER_BOOT_ARGS  *BootArgs
  ) // called as entry point
{
  EFI_STATUS            Status;

  EFI_MEMORY_DESCRIPTOR *MemMap;

  MemMap = (EFI_MEMORY_DESCRIPTOR *)(UINTN)*BootArgs->MemMap;

  // fix runtime entries
  *BootArgs->RtPageStart        -= EFI_SIZE_TO_PAGES ((UINT32)mRelocationBase);

  // VirtualPageStart is ok in boot args (a miracle!), but we'll do it anyway
  *BootArgs->RtVirtualPageStart -= EFI_SIZE_TO_PAGES ((UINT32)mRelocationBase);

  ProtectRtDataFromRelocation (*BootArgs->MemMapSize, *BootArgs->MemMapDescriptorSize, *BootArgs->MemMapDescriptorVersion, MemMap);
  AssignVirtualAddressesToMemMap (*BootArgs->MemMapSize, *BootArgs->MemMapDescriptorSize, *BootArgs->MemMapDescriptorVersion, MemMap, EFI_PAGES_TO_SIZE ((EFI_PHYSICAL_ADDRESS)*BootArgs->RtPageStart));

  Status = SetPartialVirtualAddressMap (*BootArgs->MemMapSize, *BootArgs->MemMapDescriptorSize, *BootArgs->MemMapDescriptorVersion, MemMap);

  if (!EFI_ERROR (Status)) {
    RelocateSystemTable (BootArgs->SystemTable);
    DefragmentRuntimeServices (*BootArgs->MemMapSize, *BootArgs->MemMapDescriptorSize, *BootArgs->MemMapDescriptorVersion, MemMap, BootArgs->SystemTable, FALSE);
  }
}
