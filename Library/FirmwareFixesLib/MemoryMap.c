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

#include <MiscBase.h>

#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/EfiMemoryLib.h>
#include <Library/VirtualMemoryLib.h>

// gVirtualMemMap
/// Buffer for virtual address map - only for RT areas
/// note: DescriptorSize is usually > sizeof(EFI_MEMORY_DESCRIPTOR)
/// so this Buffer can hold less then 64 descriptors
EFI_MEMORY_DESCRIPTOR gVirtualMemMap[64];

// SetPartialVirtualAddressMap
/** Copies RT flagged areas to separate Memory Map, defines virtual to phisycal address mapping
    and calls SetVirtualAddressMap() only with that partial Memory Map.

  About partial Memory Map:
  Some UEFIs are converting pointers to virtual addresses even if they do not
  point to regions with RT flag. This means that those UEFIs are using
  MemoryDescriptor->VirtualStart even for non-RT regions. Linux had issues with this:
  http://git.kernel.org/?p=linux/kernel/git/torvalds/linux-2.6.git;a=commit;h=7cb00b72876ea2451eb79d468da0e8fb9134aa8a
  They are doing it Windows way now - copying RT descriptors to separate
  mem map and passing that stripped map to SetVirtualAddressMap().
  We'll do the same, although it seems that just assigning
  VirtualStart = PhysicalStart for non-RT areas also does the job.

  About virtual to phisycal mappings:
  Also adds virtual to phisycal address mappings for RT areas. This is needed since
  SetVirtualAddressMap() does not work on my Aptio without that. Probably because some driver
  has a bug and is trying to access new virtual addresses during the change.
  Linux and Windows are doing the same thing and problem is
  not visible there.
**/
EFI_STATUS
SetPartialVirtualAddressMap (
  IN UINTN                  MemoryMapSize,
  IN UINTN                  DescriptorSize,
  IN UINT32                 DescriptorVersion,
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap
  )
{
  EFI_STATUS                 Status;

  UINTN                      Index;
  EFI_MEMORY_DESCRIPTOR      *MemoryDescriptor;
  EFI_MEMORY_DESCRIPTOR      *VirtualMemoryDescriptor;
  UINTN                      VirtualMemoryMapSize;
  PAGE_MAP_AND_DIRECTORY_PTR *PageTable;
  UINTN                      Flags;

  MemoryDescriptor        = MemoryMap;
  VirtualMemoryDescriptor = gVirtualMemMap;
  VirtualMemoryMapSize    = 0;

  GetCurrentPageTable (&PageTable, &Flags);

  for (Index = 0; Index < (MemoryMapSize / DescriptorSize); ++Index) {
    if (BIT_SET (MemoryDescriptor->Attribute, EFI_MEMORY_RUNTIME)) {
      // check if there is enough space in gVirtualMemMap

      if ((VirtualMemoryMapSize + DescriptorSize) > sizeof (gVirtualMemMap)) {
        Status = EFI_OUT_OF_RESOURCES;

        goto Return;
      }

      // copy region with EFI_MEMORY_RUNTIME flag to gVirtualMemMap
      CopyMem ((VOID *)VirtualMemoryDescriptor, (VOID *)MemoryDescriptor, DescriptorSize);

      // define virtual to phisical mapping
      VmMapVirtualPages (PageTable, MemoryDescriptor->VirtualStart, MemoryDescriptor->NumberOfPages, MemoryDescriptor->PhysicalStart);

      // next gVirtualMemMap slot
      VirtualMemoryDescriptor = NEXT_MEMORY_DESCRIPTOR (VirtualMemoryDescriptor, DescriptorSize);
      VirtualMemoryMapSize   += DescriptorSize;
    }

    MemoryDescriptor = NEXT_MEMORY_DESCRIPTOR (MemoryDescriptor, DescriptorSize);
  }

  VmFlashCaches ();

  Status = SetVirtualAddressMap (VirtualMemoryMapSize, DescriptorSize, DescriptorVersion, gVirtualMemMap);

Return:
  return Status;
}

// ProtectRtDataFromRelocation
/** Protect RT data from relocation by marking them MemoryMapIO. Except area with EFI system table.
    This one must be relocated into kernel boot image or kernel will crash (kernel accesses it
    before RT areas are mapped into vm).

  This fixes NVRAM issues on some boards where access to nvram after boot services is possible
  only in SMM mode. RT driver passes data to SM handler through previously negotiated Buffer
  and this Buffer must not be relocated.
  Explained and examined in detail by CodeRush and night199uk:
  http://www.projectosx.com/forum/index.php?showtopic=3298
  It seems this does not do any harm to others where this is not needed,
  so it's added as standard fix for all.
**/
VOID
ProtectRtDataFromRelocation (
  IN UINTN                  MemoryMapSize,
  IN UINTN                  DescriptorSize,
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                  SystemTableArea
  )
{
  UINTN Index;

  for (Index = 0; Index < (MemoryMapSize / DescriptorSize); ++Index) {
    if (BIT_SET (MemoryMap->Attribute, EFI_MEMORY_RUNTIME)
     && (MemoryMap->Type == EfiRuntimeServicesData)
     && (MemoryMap->PhysicalStart != SystemTableArea)) {
      MemoryMap->Type = EfiMemoryMappedIO;
    }

    MemoryMap = NEXT_MEMORY_DESCRIPTOR (MemoryMap, DescriptorSize);
  }
}

// DefragmentRuntimeServices
/** Copies RT code and data blocks to reserved area inside kernel boot image.
**/
VOID
DefragmentRuntimeServices (
  IN     UINTN                  MemoryMapSize,
  IN     UINTN                  DescriptorSize,
  IN     EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN OUT UINT32                 *SystemTable, OPTIONAL
  IN     EFI_PHYSICAL_ADDRESS   BaseOffset,
  IN     UINTN                  SystemTableArea OPTIONAL
  )
{
  UINTN                Index;
  EFI_PHYSICAL_ADDRESS KernelRtBlock;
  UINT64               KernelRtBlockSize;

  for (Index = 0; Index < (MemoryMapSize / DescriptorSize); ++Index) {
    // defragment only RT blocks
    // skip our block with sys table copy if required
    if (((MemoryMap->Type == EfiRuntimeServicesCode) || (MemoryMap->Type == EfiRuntimeServicesData))
     && (MemoryMap->PhysicalStart != SystemTableArea)) {
      // physical addr from virtual
      // 0x3FFFFFFFFF as in boot.efi !!
      KernelRtBlock     = (EFI_PHYSICAL_ADDRESS)APPLY_MASK (MemoryMap->VirtualStart, 0x7FFFFFFFFF);
      KernelRtBlockSize = EFI_PAGES_TO_SIZE (MemoryMap->NumberOfPages);

      CopyMem (
        (VOID *)((UINTN)KernelRtBlock + BaseOffset),
        (VOID *)(UINTN)MemoryMap->PhysicalStart,
        (UINTN)KernelRtBlockSize
        );

      // boot.efi zeros old RT areas, but we must not do that because that brakes sleep
      // on some UEFIs. why?

      if ((SystemTable != NULL)
       && (MemoryMap->PhysicalStart <= *SystemTable)
       && ((UINTN)*SystemTable < (UINTN)(MemoryMap->PhysicalStart + KernelRtBlockSize))) {
        // block contains sys table - update bootArgs with new address
        *SystemTable = (UINT32)((UINTN)KernelRtBlock + ((UINTN)*SystemTable - (UINTN)MemoryMap->PhysicalStart));
      }

      // mark old RT block in MemoryMap as ACPI NVS
      // if sleep is broken or if those areas are zeroed, maybe
      // it's safer to mark it ACPI NVS then make it free
      // and remove RT attribute
      MemoryMap->Type      = EfiACPIMemoryNVS;
      MemoryMap->Attribute = UNSET_BIT (MemoryMap->Attribute, EFI_MEMORY_RUNTIME);

    }

    MemoryMap = NEXT_MEMORY_DESCRIPTOR (MemoryMap, DescriptorSize);
  }
}

// ShrinkMemoryMap
VOID
ShrinkMemoryMap (
  IN UINTN                  *MemoryMapSize,
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                  DescriptorSize
  )
{
  UINTN                 OffsetToEnd;
  EFI_MEMORY_DESCRIPTOR *MemoryDescriptor;
  BOOLEAN               MergeDescriptors;

  MemoryDescriptor = NEXT_MEMORY_DESCRIPTOR (MemoryMap, DescriptorSize);
  OffsetToEnd      = (*MemoryMapSize - DescriptorSize);
  *MemoryMapSize   = DescriptorSize;
  MergeDescriptors = FALSE;

  while (OffsetToEnd > 0) {
    if ((MemoryDescriptor->Attribute == MemoryMap->Attribute)
     && ((MemoryMap->PhysicalStart + EFI_PAGES_TO_SIZE (MemoryMap->NumberOfPages)) == MemoryDescriptor->PhysicalStart)
     && ((MemoryDescriptor->Type == EfiBootServicesCode) || (MemoryDescriptor->Type == EfiBootServicesData))
     && ((MemoryMap->Type == EfiBootServicesCode) || (MemoryMap->Type == EfiBootServicesData))) {
      // two entries are the same/similar - join them
      MemoryMap->NumberOfPages += MemoryDescriptor->NumberOfPages;
      MergeDescriptors          = TRUE;
    } else {
      // can not be joined - we need to move to next
      *MemoryMapSize += DescriptorSize;
      MemoryMap       = NEXT_MEMORY_DESCRIPTOR (MemoryMap, DescriptorSize);

      if (MergeDescriptors) {
        // have entries between MemoryMap and MemoryDescriptor which are joined to MemoryMap
        // we need to copy [MemoryDescriptor, end of list] to MemoryMap + 1
        CopyMem ((VOID *)MemoryMap, (VOID *)MemoryDescriptor, OffsetToEnd);

        MemoryDescriptor = MemoryMap;
        MergeDescriptors = FALSE;
      }
    }

    MemoryDescriptor = NEXT_MEMORY_DESCRIPTOR (MemoryDescriptor, DescriptorSize);
    OffsetToEnd     -= DescriptorSize;
  }
}

// FixMemoryMap
VOID
FixMemoryMap (
  IN UINTN                  MemoryMapSize,
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                  DescriptorSize
  )
{
  UINTN Index;

  for (Index = 0; Index < (MemoryMapSize / DescriptorSize); ++Index) {
    // Some UEFIs end up with "reserved" area with EFI_MEMORY_RUNTIME flag set
    // when Intel HD3000 or HD4000 is used. We will remove that flag here.
    if (BIT_SET (MemoryMap->Attribute, EFI_MEMORY_RUNTIME) && (MemoryMap->Type == EfiReservedMemoryType)) {
      MemoryMap->Attribute = UNSET_BIT (MemoryMap->Attribute, EFI_MEMORY_RUNTIME);
    }

    // Fix by Slice - fixes sleep/wake on GB boards.
    if ((MemoryMap->PhysicalStart < 0x0A0000)
     && ((MemoryMap->PhysicalStart + EFI_PAGES_TO_SIZE (MemoryMap->NumberOfPages)) >= 0x09E000)) {
      MemoryMap->Type      = EfiACPIMemoryNVS;
      MemoryMap->Attribute = 0;
    }

    MemoryMap = NEXT_MEMORY_DESCRIPTOR (MemoryMap, DescriptorSize);
  }
}
