/** @file
  Copyright (C) 2017, CupertinoNet.  All rights reserved.<BR>

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/PeCoffLib.h>
#include <Library/XnuSupportMemoryAllocationLib.h>

LIST_ENTRY gNotifys = INITIALIZE_LIST_HEAD_VARIABLE (gNotifys);

UINTN
LoadKernelEntryNotifyImage (
  IN VOID  *FileBuffer
  )
{
  UINTN                        EntryPoint;

  PE_COFF_LOADER_IMAGE_CONTEXT ImageContext;
  EFI_STATUS                   Status;
  UINTN                        Size;

  EntryPoint = 0;

  ZeroMem ((VOID *)&ImageContext, sizeof (ImageContext));

  ImageContext.Handle    = FileBuffer;
  ImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;

  Status = PeCoffLoaderGetImageInfo (&ImageContext);

  if (!EFI_ERROR (Status)) {
    ASSERT (!ImageContext.IsTeImage);
    ASSERT (!ImageContext.RelocationsStripped);
    ASSERT (EFI_IMAGE_MACHINE_TYPE_SUPPORTED (ImageContext.Machine)
         || EFI_IMAGE_MACHINE_CROSS_TYPE_SUPPORTED (ImageContext.Machine));

    if (!ImageContext.IsTeImage
     && !ImageContext.RelocationsStripped
     && (!EFI_IMAGE_MACHINE_TYPE_SUPPORTED (ImageContext.Machine)
      || !EFI_IMAGE_MACHINE_CROSS_TYPE_SUPPORTED (ImageContext.Machine))) {
      ImageContext.ImageCodeMemoryType = EfiBootServicesCode;
      ImageContext.ImageDataMemoryType = EfiBootServicesData;

      Size = (UINTN)ImageContext.ImageSize;

      if (ImageContext.SectionAlignment > EFI_PAGE_SIZE) {
        Size += ImageContext.SectionAlignment;
      }

      ImageContext.ImageAddress = (PHYSICAL_ADDRESS)(UINTN)(
                                    AllocateXnuSupportCode (Size)
                                    );

      if (ImageContext.ImageAddress != 0) {
        ImageContext.ImageAddress = ALIGN_VALUE (
                                      ImageContext.ImageAddress,
                                      ImageContext.SectionAlignment
                                      );

        Status = PeCoffLoaderLoadImage (&ImageContext);

        if (!EFI_ERROR (Status)) {
          Status = PeCoffLoaderRelocateImage (&ImageContext);

          if (!EFI_ERROR (Status)) {
            InvalidateInstructionCacheRange (
              (VOID *)(UINTN)ImageContext.ImageAddress,
              (UINTN)ImageContext.ImageSize
              );

            EntryPoint = (UINTN)ImageContext.EntryPoint;

            goto Done;
          }
        }

        FreeXnuSupportMemory (
          (VOID *)(UINTN)ImageContext.ImageAddress,
          EFI_SIZE_TO_PAGES (Size)
          );
      }
    }
  }

Done:
  return EntryPoint;
}
