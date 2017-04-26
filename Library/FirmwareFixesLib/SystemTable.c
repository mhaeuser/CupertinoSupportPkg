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

// mSTCopy
STATIC EFI_SYSTEM_TABLE *mSTCopy = NULL;

// mAppleBooterLevel
STATIC INTN mAppleBooterLevel = 0;

// InternalOverrideSystemTable
VOID
InternalOverrideSystemTable (
  IN VOID  *Registration
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                BooterHandle;
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;

  ASSERT (Registration != NULL);

  ++mAppleBooterLevel;

  DEBUG_CODE (
    ASSERT (
      (((mAppleBooterLevel == 1) ? 1 : 0) ^ ((mSTCopy != NULL) ? 1 : 0)) != 0
      );
    );

  if (mAppleBooterLevel == 1) {
    mSTCopy = AllocateXnuSupportData (gST->Hdr.HeaderSize);
  }

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

  Status = EfiHandleProtocol (
             BooterHandle,
             &gEfiLoadedImageProtocolGuid,
             (VOID **)&LoadedImage
             );

  ASSERT (Status != EFI_UNSUPPORTED);

  LoadedImage->SystemTable = mSTCopy;
}

// InternalFreeSystemTableCopy
VOID
InternalFreeSystemTableCopy (
  VOID
  )
{
  --mAppleBooterLevel;

  if (mAppleBooterLevel == 0) {
    FreeXnuSupportMemory (
      (VOID *)mSTCopy,
      EFI_SIZE_TO_PAGES (mSTCopy->Hdr.HeaderSize)
    );

    DEBUG_CODE (
      mSTCopy = NULL;
    );
  }
}
