/** @file
  Copyright (C) 2012 modbin.  All rights reserved.<BR>
  Portions Copyright (C) 2012 - 2014 Damir Mažar.  All rights reserved.<BR>
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

#include <Library/CupertinoXnuLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MiscMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include "FirmwareFixesInternal.h"

// FreeLowMemory
/** Free all memory pages in the XNU Kernel Address Space.
**/
VOID
FreeXnuMemorySpace (
  VOID
  )
{
  UINTN                 MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR *MemoryMap;
  UINTN                 MapKey;
  UINTN                 DescriptorSize;
  UINT32                DescriptorVersion;
  UINTN                 MemoryMapTop;
  EFI_MEMORY_DESCRIPTOR *MemoryDescriptor;
  EFI_PHYSICAL_ADDRESS  MemoryStart;
  EFI_PHYSICAL_ADDRESS  MemoryTop;

  MemoryMap = GetMemoryMapBuffer (
                gBS->GetMemoryMap,
                &MemoryMapSize,
                &MapKey,
                &DescriptorSize,
                &DescriptorVersion
                );

  if (MemoryMap != NULL) {
    MemoryMapTop     = ((UINTN)MemoryMap + MemoryMapSize);
    MemoryDescriptor = MemoryMap;

    while ((UINTN)MemoryDescriptor < MemoryMapTop) {
      if ((MemoryDescriptor->Type != EfiConventionalMemory)) {
        MemoryStart = MemoryDescriptor->PhysicalStart;

        if (MemoryStart <= XNU_KERNEL_MAX_PHYSICAL_ADDRESS) {
          MemoryTop = MEMORY_DESCRIPTOR_PHYSICAL_TOP (MemoryDescriptor);

          if (MemoryTop > XNU_KERNEL_MIN_PHYSICAL_ADDRESS) {
            FreePages (
              (VOID *)(UINTN)MemoryDescriptor->PhysicalStart,
              (UINTN)MemoryDescriptor->NumberOfPages
              );
          }
        }
      }

      MemoryDescriptor = NEXT_MEMORY_DESCRIPTOR (
                           MemoryDescriptor,
                           DescriptorSize
                           );
    }

    FreePool ((VOID *)MemoryMap);
  }
}
