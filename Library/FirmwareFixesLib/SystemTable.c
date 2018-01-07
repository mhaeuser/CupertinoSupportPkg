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

#include <Protocol/AppleBooterHandle.h>
#include <Protocol/LoadedImage.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/EfiBootServicesLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/XnuSupportMemoryAllocationLib.h>

#include "FirmwareFixesInternal.h"

STATIC EFI_SYSTEM_TABLE *mSTCopy = NULL;

VOID
InternalFreeSystemTableCopy (
  VOID
  )
{
  if (mSTCopy != NULL) {
    FreeXnuSupportMemory (
      (VOID *)mSTCopy,
      mSTCopy->Hdr.HeaderSize
      );

    mSTCopy = NULL;
  }
}

VOID
InternalOverrideSystemTable (
  IN VOID  *Registration
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                BooterHandle;
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;

  ASSERT (Registration != NULL);

  if (mSTCopy == NULL) {
    mSTCopy = AllocateXnuSupportData (gST->Hdr.HeaderSize);

    if (mSTCopy != NULL) {
      CopyMem (
        (VOID *)mSTCopy,
        (VOID *)gST,
        gST->Hdr.HeaderSize
        );

      Status = EfiLocateProtocol (
                 &gAppleBooterHandleProtocolGuid,
                 Registration,
                 (VOID **)&BooterHandle
                 );

      ASSERT (Status != EFI_NOT_FOUND);

      if (!EFI_ERROR (Status)) {
        Status = EfiHandleProtocol (
                   BooterHandle,
                   &gEfiLoadedImageProtocolGuid,
                   (VOID **)&LoadedImage
                   );

        ASSERT (Status != EFI_UNSUPPORTED);

        if (!EFI_ERROR (Status)) {
          LoadedImage->SystemTable = mSTCopy;
        }
      }

      if (EFI_ERROR (Status)) {
        InternalFreeSystemTableCopy ();
      }
    }
  }
}
