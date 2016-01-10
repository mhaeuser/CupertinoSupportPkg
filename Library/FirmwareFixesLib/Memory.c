/** @file
  Copyright (C) 2012 - 2014 Damir Mazar.  All rights reserved.<BR>
  Portions Copyright (C) 2012 modbin.  All rights reserved.<BR>
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

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/AppleXnuLib.h>

#include "FirmwareFixes.h"

// PREV_MEMORY_DESCRIPTOR
/** Macro that returns the a pointer to the next EFI_MEMORY_DESCRIPTOR in an array 
    returned from GetMemoryMap().  

  @param[in] MemoryDescriptor  A pointer to an EFI_MEMORY_DESCRIPTOR.

  @param[in] Size              The size, in bytes, of the current EFI_MEMORY_DESCRIPTOR.

  @return A pointer to the next EFI_MEMORY_DESCRIPTOR.
**/
#define PREV_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_MEMORY_DESCRIPTOR *)((UINTN)(MemoryDescriptor) - (Size)))

// AllocatePagesFromTop
/** Alloctes Pages from the top of mem, up to address specified in Memory. Returns allocated address in Memory.
**/
EFI_STATUS
EFIAPI
AllocatePagesFromTop (
  IN EFI_MEMORY_TYPE           MemoryType,
  IN UINTN                     Pages,
  IN OUT EFI_PHYSICAL_ADDRESS  *Memory
  )
{
  EFI_STATUS            Status;

  UINTN                 MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR *MemoryMap;
  UINTN                 MapKey;
  UINTN                 DescriptorSize;
  UINT32                DescriptorVersion;
  EFI_MEMORY_DESCRIPTOR *MemoryMapEnd;
  EFI_MEMORY_DESCRIPTOR *Desc;

  Status    = EFI_NOT_FOUND;
  MemoryMap = GetMemoryMapBuffer (gBS->GetMemoryMap, &MemoryMapSize, &MapKey, &DescriptorSize, &DescriptorVersion);

  if (MemoryMap != NULL) {
    MemoryMapEnd = NEXT_MEMORY_DESCRIPTOR (MemoryMap, MemoryMapSize);
    Desc         = PREV_MEMORY_DESCRIPTOR (MemoryMapEnd, DescriptorSize);

    while ((UINTN)Desc >= (UINTN)MemoryMap) {
      if ((Desc->Type == EfiConventionalMemory) && (Pages <= Desc->NumberOfPages) && ((Desc->PhysicalStart + EFI_PAGES_TO_SIZE (Pages)) <= *Memory)) {
        // free block found
        if (Desc->PhysicalStart + EFI_PAGES_TO_SIZE ((UINTN)Desc->NumberOfPages) <= *Memory) {
          // the whole block is unded Memory - allocate from the top of the block
          *Memory = (Desc->PhysicalStart + EFI_PAGES_TO_SIZE ((UINTN)Desc->NumberOfPages - Pages));
        } else {
          // the block contains enough pages under Memory, but spans above it - allocate below Memory.
          *Memory -= EFI_PAGES_TO_SIZE (Pages);
        }

        Status = gBS->AllocatePages (AllocateAddress, MemoryType, Pages, (VOID *)Memory);

        break;
      }

      Desc = PREV_MEMORY_DESCRIPTOR (Desc, DescriptorSize);
    }

    // release mem
    FreePool ((VOID *)MemoryMap);
  }

  return Status;
}

// GetNoRuntimePages
/** Helper function that calculates number of RT and MMIO pages from mem map.
**/
EFI_STATUS
GetNoRuntimePages (
  OUT UINTN  *NoPages
  )
{
  EFI_STATUS            Status;

  UINTN                 MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR *MemoryMap;
  UINTN                 MapKey;
  UINTN                 DescriptorSize;
  UINT32                DescriptorVersion;
  UINTN                 Index;

  Status = GetMemoryMapBuffer (gBS->GetMemoryMap, &MemoryMapSize, &MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);

  if (!EFI_ERROR (Status)) {
    FixMemoryMap (MemoryMapSize, MemoryMap, DescriptorSize, DescriptorVersion);

    // Sum RT and MMIO areas - all that have runtime attribute
    *NoPages = 0;

    for (Index = 0; Index < (MemoryMapSize / DescriptorSize); ++Index) {
      if (BIT_SET (MemoryMap->Attribute, EFI_MEMORY_RUNTIME)) {
        *NoPages += MemoryMap->NumberOfPages;
      }

      MemoryMap = NEXT_MEMORY_DESCRIPTOR (MemoryMap, DescriptorSize);
    }
  }

  return Status;
}

// FreeLowMemory
/** Free all mem regions between 0x100000 and KERNEL_TOP_ADDRESS
**/
VOID
FreeLowMemory (
  VOID
  )
{
  EFI_STATUS            Status;
  UINTN                 MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR *MemoryMap;
  UINTN                 MapKey;
  UINTN                 DescriptorSize;
  UINT32                DescriptorVersion;
  EFI_MEMORY_DESCRIPTOR *MemoryMapEnd;
  EFI_MEMORY_DESCRIPTOR *Desc;

  Status = GetMemoryMapBuffer (
             gBS->GetMemoryMap,
             &MemoryMapSize,
             &MemoryMap,
             &MapKey,
             &DescriptorSize,
             &DescriptorVersion
             );

  if (!EFI_ERROR (Status)) {
    MemoryMapEnd = NEXT_MEMORY_DESCRIPTOR (MemoryMap, MemoryMapSize);

    for (Desc = MemoryMap; Desc < MemoryMapEnd; Desc = NEXT_MEMORY_DESCRIPTOR (Desc, DescriptorSize)) {
      if (Desc->Type != EfiConventionalMemory
       && Desc->PhysicalStart + EFI_PAGES_TO_SIZE ((UINTN)Desc->NumberOfPages) > 0x100000
       && Desc->PhysicalStart < APPLE_XNU_MAX_ADDRESS
        ) {
        FreePages ((VOID *)Desc->PhysicalStart, Desc->NumberOfPages);
      }
    }

    FreePool ((VOID *)MemoryMap);
  }
}
