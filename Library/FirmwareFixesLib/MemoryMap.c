/** @file
  Copyright (C) 2012 - 2014 Damir Mažar.  All rights reserved.<BR>
  Portions Copyright (C) 2015 - 2017, CupertinoNet.  All rights reserved.<BR>

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

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/VirtualMemoryLib.h>

#include "FirmwareFixesInternal.h"

// TODO: We can just catch the XnuPrepareStart event to allocate this
//       dynamically.
///
/// Buffer for virtual address map - only for RT areas
/// Note: DescriptorSize is usually > sizeof (EFI_MEMORY_DESCRIPTOR), so this
/// Buffer can most likely only hold less then 64 descriptors
///
STATIC EFI_MEMORY_DESCRIPTOR mVirtualAddressMap[64];

/**
  Copies RT flagged areas to separate Memory Map, defines virtual to phisycal
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

  @param[in] MemoryMapSize   The size in bytes of VirtualMap.
  @param[in] DescriptorSize  The size in bytes of an entry in the VirtualMap.
  @param[in] VirtualMap      An array of memory descriptors which contain new
                             virtual address mapping information for all
                             runtime ranges.

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
    if ((MemoryDescriptor->Attribute & EFI_MEMORY_RUNTIME) != 0) {
      if ((VirtualMemoryMapSize + DescriptorSize) > sizeof (mVirtualAddressMap)) {
        VirtualAddressMap = NULL;

        ASSERT (FALSE);

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

/**
  Adds virtual to phisycal address mappings for RT areas. This is needed since
  SetVirtualAddressMap() does not work on my Aptio without that.
  Probably because some driver has a bug and is trying to access new virtual
  addresses during the change.  Linux and Windows are doing the same thing and
  problem is not visible there.

  @param[in] MemoryMapSize   The size in bytes of VirtualMap.
  @param[in] DescriptorSize  The size in bytes of an entry in the VirtualMap.
  @param[in] VirtualMap      An array of memory descriptors which contain new
                             virtual address mapping information for all
                             runtime ranges.
 
**/
VOID
MapVirtualPages (
  IN UINTN                  MemoryMapSize,
  IN UINTN                  DescriptorSize,
  IN EFI_MEMORY_DESCRIPTOR  *VirtualMap
  )
{
  EFI_MEMORY_DESCRIPTOR *MemoryDescriptor;
  VOID                  *PageTable;
  UINTN                  Index;

  ASSERT (MemoryMapSize > 0);
  ASSERT (DescriptorSize > 0);
  ASSERT (DescriptorSize < MemoryMapSize);
  ASSERT ((MemoryMapSize % DescriptorSize) == 0);
  ASSERT (VirtualMap != NULL);

  MemoryDescriptor = VirtualMap;

  PageTable = VirtualMemoryGetPageTable (NULL);

  for (Index = 0; Index < (MemoryMapSize / DescriptorSize); ++Index) {
    VirtualMemoryMapVirtualPages (
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

  VirtualMemoryFlashCaches ();
}

/**
  Protect RT data from relocation by marking them MemoryMapIO. Except area with
  EFI system table.  This one must be relocated into kernel boot image or
  kernel will crash (kernel accesses it before RT areas are mapped into vm).

  This fixes NVRAM issues on some boards where access to nvram after boot
  services is possible only in SMM mode. RT driver passes data to SM handler
  through previously negotiated Buffer and this Buffer must not be relocated.
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
    if (((MemoryMap->Attribute & EFI_MEMORY_RUNTIME) != 0)
     && (MemoryMap->Type == EfiRuntimeServicesData)
     && (MemoryMap->PhysicalStart != SystemTableArea)) {
      MemoryMap->Type = EfiMemoryMappedIO;
    }

    MemoryMap = NEXT_MEMORY_DESCRIPTOR (MemoryMap, DescriptorSize);
  }
}

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
      MemoryMapWalker->Type           = EfiConventionalMemory;
      MemoryMapWalker->NumberOfPages += MemoryDescriptor->NumberOfPages;
      MergeDescriptors                = TRUE;
    } else {
      if (MergeDescriptors) {
        //
        // We have entries between MemoryMap and MemoryDescriptor which are to
        // be joined.
        // We need to copy [MemoryDescriptor, end of list] to MemoryMap + 1.
        //
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
    //
    // Some UEFIs end up with a "Reserved" area with EFI_MEMORY_RUNTIME flag
    // set when Intel HD3000 or HD4000 is used.  As earlier version of boot.efi
    // ignored such regions, they were not assigned a virtual address, though
    // the kernel tried to map it nevertheless.
    // Set the type to MMIO to have it assigned a virtual address correctly.
    //
    if (((MemoryMapWalker->Attribute & EFI_MEMORY_RUNTIME) != 0)
     && (MemoryMapWalker->Type == EfiReservedMemoryType)) {
      MemoryMapWalker->Type = EfiConventionalMemory;
    }

    // TODO: Why is this needed?
#if 0
    //
    // Fix by Slice - fixes sleep/wake on GB boards.
    //
    if ((MemoryMapWalker->PhysicalStart < 0x0A0000)
     && (MEMORY_DESCRIPTOR_PHYSICAL_TOP (MemoryMapWalker) >= 0x09E000)) {
      MemoryMapWalker->Type      = EfiACPIMemoryNVS;
      MemoryMapWalker->Attribute = 0;
    }
#endif

    MemoryMapWalker = NEXT_MEMORY_DESCRIPTOR (MemoryMapWalker, DescriptorSize);
  }
}
