/** @file
  Copyright (C) 2012 - 2014 Damir Mažar.  All rights reserved.<BR>
  Portions Copyright (C) 2015 - 2016, CupertinoNet.  All rights reserved.<BR>

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
#include <Library/CupertinoXnuLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/VirtualMemoryLib.h>

#include "FirmwareFixesInternal.h"

// mVirtualAddressMap
/// Buffer for virtual address map - only for RT areas
/// note: DescriptorSize is usually > sizeof(EFI_MEMORY_DESCRIPTOR)
/// so this Buffer can hold less then 64 descriptors
STATIC EFI_MEMORY_DESCRIPTOR mVirtualAddressMap[64];

// GetPartialVirtualAddressMap
/** Copies RT flagged areas to separate Memory Map, defines virtual to phisycal
    address mapping and calls SetVirtualAddressMap() only with that partial
    Memory Map.

  About partial Memory Map:
  Some UEFIs are converting pointers to virtual addresses even if they do not
  point to regions with RT flag.  This means that those UEFIs are using
  MemoryDescriptor->VirtualStart even for non-RT regions.  Linux had issues
  with this:
  http://git.kernel.org/?p=linux/kernel/git/torvalds/linux-2.6.git;a=commit;h=7cb00b72876ea2451eb79d468da0e8fb9134aa8a
  They are doing it Windows way now - copying RT descriptors to separate Memory
  Map and passing that stripped map to SetVirtualAddressMap().  We'll do the
  same, although it seems that just assigning VirtualStart = PhysicalStart for
  non-RT areas also does the job.

  About virtual to phisycal mappings:
  Also adds virtual to phisycal address mappings for RT areas. This is needed
  since SetVirtualAddressMap() does not work on my Aptio without that.
  Probably because some driver has a bug and is trying to access new virtual
  addresses during the change.  Linux and Windows are doing the same thing and
  problem is not visible there.
**/
EFI_MEMORY_DESCRIPTOR *
GetPartialVirtualAddressMap (
  IN UINTN                  MemoryMapSize,
  IN UINTN                  DescriptorSize,
  IN EFI_MEMORY_DESCRIPTOR  *VirtualMap
  )
{
  EFI_MEMORY_DESCRIPTOR *VirtualAddressMap;

  EFI_MEMORY_DESCRIPTOR *MemoryDescriptor;
  EFI_MEMORY_DESCRIPTOR *VirtualMemoryDescriptor;
  UINTN                 VirtualMemoryMapSize;
  UINTN                 Index;

  ASSERT (MemoryMapSize > 0);
  ASSERT (DescriptorSize > 0);
  ASSERT (DescriptorSize <= MemoryMapSize);
  ASSERT ((MemoryMapSize % DescriptorSize) == 0);
  ASSERT (VirtualMap != NULL);

  MemoryDescriptor        = VirtualMap;
  VirtualMemoryDescriptor = mVirtualAddressMap;
  VirtualMemoryMapSize    = 0;

  VirtualAddressMap = mVirtualAddressMap;

  for (Index = 0; Index < (MemoryMapSize / DescriptorSize); ++Index) {
    if (BIT_SET (MemoryDescriptor->Attribute, EFI_MEMORY_RUNTIME)) {
      if ((VirtualMemoryMapSize + DescriptorSize) > sizeof (mVirtualAddressMap)) {
        VirtualAddressMap = NULL;

        ASSERT (TRUE);

        break;
      }

      CopyMem (
        (VOID *)VirtualMemoryDescriptor,
        (VOID *)MemoryDescriptor,
        DescriptorSize
        );

      VirtualMemoryDescriptor = NEXT_MEMORY_DESCRIPTOR (
                                  VirtualMemoryDescriptor,
                                  DescriptorSize
                                  );

      VirtualMemoryMapSize += DescriptorSize;
    }

    MemoryDescriptor = NEXT_MEMORY_DESCRIPTOR (
                         MemoryDescriptor,
                         DescriptorSize
                         );
  }

  return VirtualAddressMap;
}

VOID
MapVirtualPages (
  IN UINTN                  MemoryMapSize,
  IN UINTN                  DescriptorSize,
  IN EFI_MEMORY_DESCRIPTOR  *VirtualMap
  )
{
  EFI_MEMORY_DESCRIPTOR      *MemoryDescriptor;
  PAGE_MAP_AND_DIRECTORY_PTR *PageTable;
  UINTN                      Index;

  ASSERT (MemoryMapSize > 0);
  ASSERT (DescriptorSize > 0);
  ASSERT (DescriptorSize < MemoryMapSize);
  ASSERT ((MemoryMapSize % DescriptorSize) == 0);
  ASSERT (VirtualMap != NULL);

  MemoryDescriptor = VirtualMap;

  PageTable = GetCurrentPageTable (NULL);

  for (Index = 0; Index < (MemoryMapSize / DescriptorSize); ++Index) {
    VmMapVirtualPages (
      PageTable,
      MemoryDescriptor->VirtualStart,
      MemoryDescriptor->NumberOfPages,
      MemoryDescriptor->PhysicalStart
      );

    MemoryDescriptor = NEXT_MEMORY_DESCRIPTOR (
                         MemoryDescriptor,
                         DescriptorSize
                         );
  }

  VmFlashCaches ();
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
ProtectRutimeDataFromRelocation (
  IN UINTN                  MemoryMapSize,
  IN UINTN                  DescriptorSize,
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                  SystemTableArea
  )
{
  UINTN Index;

  ASSERT (MemoryMapSize > 0);
  ASSERT (DescriptorSize > 0);
  ASSERT ((MemoryMapSize % DescriptorSize) == 0);
  ASSERT (MemoryMap != NULL);
  ASSERT (SystemTableArea != 0);

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
  UINTN  Index;
  UINTN  KernelRtBlock;
  UINT64 KernelRtBlockSize;

  for (Index = 0; Index < (MemoryMapSize / DescriptorSize); ++Index) {
    // defragment only RT blocks
    // skip our block with sys table copy if required
    if (((MemoryMap->Type == EfiRuntimeServicesCode)
      || (MemoryMap->Type == EfiRuntimeServicesData))
     && (MemoryMap->PhysicalStart != SystemTableArea)) {
      KernelRtBlock     = (UINTN)XNU_KERNEL_TO_PHYSICAL (MemoryMap->VirtualStart);
      KernelRtBlockSize = EFI_PAGES_TO_SIZE (MemoryMap->NumberOfPages);

      CopyMem (
        (VOID *)(UINTN)(KernelRtBlock + BaseOffset),
        (VOID *)(UINTN)MemoryMap->PhysicalStart,
        (UINTN)KernelRtBlockSize
        );

      // boot.efi zeros old RT areas, but we must not do that because that brakes sleep
      // on some UEFIs. why?

      if ((SystemTable != NULL)
       && (MemoryMap->PhysicalStart <= *SystemTable)
       && ((UINTN)*SystemTable < (UINTN)(MemoryMap->PhysicalStart + KernelRtBlockSize))) {
        // block contains sys table - update bootArgs with new address
        *SystemTable = (UINT32)(KernelRtBlock + ((UINTN)*SystemTable - (UINTN)MemoryMap->PhysicalStart));
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
  EFI_MEMORY_DESCRIPTOR *MemoryMapWalker;
  EFI_MEMORY_DESCRIPTOR *MemoryDescriptor;
  BOOLEAN               MergeDescriptors;

  MemoryMapWalker  = MemoryMap;
  MemoryDescriptor = NEXT_MEMORY_DESCRIPTOR (MemoryMap, DescriptorSize);
  OffsetToEnd      = (*MemoryMapSize - DescriptorSize);
  *MemoryMapSize   = DescriptorSize;
  MergeDescriptors = FALSE;

  while (OffsetToEnd > 0) {
    if ((MemoryMapWalker->Attribute == MemoryDescriptor->Attribute)
     && (MEMORY_DESCRIPTOR_PHYSICAL_TOP (MemoryMapWalker) == MemoryDescriptor->PhysicalStart)
     && ((MemoryDescriptor->Type == EfiBootServicesCode)
      || (MemoryDescriptor->Type == EfiBootServicesData)
      || (MemoryDescriptor->Type == EfiConventionalMemory))
     && ((MemoryMapWalker->Type  == EfiBootServicesCode)
      || (MemoryMapWalker->Type  == EfiBootServicesData)
      || (MemoryMapWalker->Type  == EfiConventionalMemory))) {
      MemoryMapWalker->NumberOfPages += MemoryDescriptor->NumberOfPages;
      MergeDescriptors                = TRUE;
    } else {
      if (MergeDescriptors) {
        // have entries between MemoryMap and MemoryDescriptor which are joined
        // to MemoryMap
        // we need to copy [MemoryDescriptor, end of list] to MemoryMap + 1
        CopyMem (
          (VOID *)MemoryMapWalker,
          (VOID *)MemoryDescriptor,
          OffsetToEnd
          );

        MemoryDescriptor = MemoryMapWalker;
        MergeDescriptors = FALSE;
      }

      *MemoryMapSize += DescriptorSize;
      MemoryMapWalker = NEXT_MEMORY_DESCRIPTOR (
                          MemoryMapWalker,
                          DescriptorSize
                          );
    }

    MemoryDescriptor = NEXT_MEMORY_DESCRIPTOR (
                         MemoryDescriptor,
                         DescriptorSize
                         );

    OffsetToEnd -= DescriptorSize;
  }
}

// FixMemoryMap
VOID
FixMemoryMap (
  IN     UINTN                  MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN     UINTN                  DescriptorSize
  )
{
  UINTN                 Index;

  EFI_MEMORY_DESCRIPTOR *MemoryMapWalker;

  MemoryMapWalker = MemoryMap;

  for (Index = 0; Index < (MemoryMapSize / DescriptorSize); ++Index) {
    // Some UEFIs end up with "reserved" area with EFI_MEMORY_RUNTIME flag set
    // when Intel HD3000 or HD4000 is used. We will remove that flag here.

    if (BIT_SET (MemoryMapWalker->Attribute, EFI_MEMORY_RUNTIME)
     && (MemoryMapWalker->Type == EfiReservedMemoryType)) {
      MemoryMapWalker->Attribute = UNSET_BIT (MemoryMapWalker->Attribute, EFI_MEMORY_RUNTIME);
    }

    // Fix by Slice - fixes sleep/wake on GB boards.

    if ((MemoryMapWalker->PhysicalStart < 0x0A0000)
     && (MEMORY_DESCRIPTOR_PHYSICAL_TOP (MemoryMapWalker) >= 0x09E000)) {
      MemoryMapWalker->Type      = EfiACPIMemoryNVS;
      MemoryMapWalker->Attribute = 0;
    }

    MemoryMapWalker = NEXT_MEMORY_DESCRIPTOR (MemoryMapWalker, DescriptorSize);
  }
}

/** Assignes OSX virtual addresses to runtime areas in memory map. */
VOID
AssignVirtualAddressesToMemoryMap (
  IN UINTN                  MemoryMapSize,
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                  DescriptorSize,
  IN UINT32                 DescriptorVersion,
  IN EFI_PHYSICAL_ADDRESS   KernelRuntimeStart
  )
{
  EFI_PHYSICAL_ADDRESS  KernelRuntime;
  EFI_MEMORY_DESCRIPTOR *MemoryDescriptor;
  UINTN                 Index;
  UINT64                MemorySize;
  
  KernelRuntime    = KernelRuntimeStart;
  MemoryDescriptor = MemoryMap;

  for (Index = 0; Index < (MemoryMapSize / DescriptorSize); ++Index) {
    MemorySize = EFI_PAGES_TO_SIZE (MemoryDescriptor->NumberOfPages);

    if ((MemoryDescriptor->Attribute & EFI_MEMORY_RUNTIME) != 0) {
      MemoryDescriptor->VirtualStart = XNU_KERNEL_TO_VIRTUAL (KernelRuntime);

      KernelRuntime += MemorySize;
    }
    
    MemoryDescriptor = NEXT_MEMORY_DESCRIPTOR (
                         MemoryDescriptor,
                         DescriptorSize
                         );
  }
}
