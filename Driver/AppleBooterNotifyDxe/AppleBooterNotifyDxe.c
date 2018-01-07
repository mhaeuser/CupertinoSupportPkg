/** @file
  Copyright (C) 2012 - 2014 Damir Mažar.  All rights reserved.<BR>
  Portions Copyright (C) 2017, CupertinoNet.  All rights reserved.<BR>

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

#include <AppleMacEfi.h>

#include <Guid/AppleBooterExitNamedEvent.h>
#include <Guid/AppleOSLoaded.h>

#include <Protocol/AppleBooterHandle.h>
#include <Protocol/LoadedImage.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/EfiBootServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MiscDevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

STATIC EFI_IMAGE_START mStartImage = NULL;

/**
  Transfers control to a loaded image's entry point.

  @param[in]  ImageHandle   Handle of image to be started.
  @param[out] ExitDataSize  The pointer to the size, in bytes, of ExitData.
  @param[out] ExitData      The pointer to a pointer to a data buffer that
                            includes a Null-terminated string, optionally
                            followed by additional binary data.

  @retval EFI_INVALID_PARAMETER   ImageHandle is either an invalid image handle
                                  or the image has already been initialized
                                  with StartImage.
  @retval EFI_SECURITY_VIOLATION  The current platform policy specifies that
                                  the image should not be started.
  @return  Exit code from image

**/
STATIC
EFI_STATUS
EFIAPI
InternalStartImage (
  IN  EFI_HANDLE  ImageHandle,
  OUT UINTN       *ExitDataSize,
  OUT CHAR16      **ExitData OPTIONAL
  )
{
  EFI_STATUS                Status;

  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
  CHAR16                    *FilePath;
  UINTN                     FilePathSize;
  UINTN                     Index;
  UINTN                     Result;
  BOOLEAN                   AppleOs;

  ASSERT (mStartImage != NULL);

  Status = EfiHandleProtocol (
             ImageHandle,
             &gEfiLoadedImageProtocolGuid,
             (VOID **)&LoadedImage
             );

  if (!EFI_ERROR (Status)) {
    FilePath = MiscFileDevicePathToText (LoadedImage->FilePath, &FilePathSize);

    if (FilePath != NULL) {
      FilePathSize -= sizeof (APPLE_BOOTER_FILE_NAME);
      Index         = (FilePathSize / sizeof (FilePath[0]));
      Result        = CompareMem (
                        (VOID *)&FilePath[Index],
                        (VOID *)APPLE_BOOTER_FILE_NAME,
                        sizeof (APPLE_BOOTER_FILE_NAME)
                        );

      FreePool ((VOID *)FilePath);

      AppleOs = (BOOLEAN)(Result == 0);

      if (AppleOs) {
        EfiInstallMultipleProtocolInterfaces (
          &gImageHandle,
          &gAppleBooterHandleProtocolGuid,
          (VOID *)ImageHandle,
          NULL
          );

        EfiUninstallMultipleProtocolInterfaces (
          gImageHandle,
          &gAppleBooterHandleProtocolGuid,
          (VOID *)ImageHandle,
          NULL
          );

        if (PcdGetBool (PcdSignalAppleOSLoadedEvent)) {
          EfiNamedEventSignal (&gAppleOSLoadedNamedEventGuid);
        }
      }
      
      Status = mStartImage (
                 ImageHandle,
                 ExitDataSize,
                 ExitData
                 );

      if (AppleOs) {
        EfiNamedEventSignal (&gAppleBooterExitNamedEventGuid);
      }
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
AppleBooterNotifyMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_TPL OldTpl;

  DEBUG_CODE (
    ASSERT (mStartImage == NULL);
    );

  OldTpl = EfiRaiseTPL (TPL_HIGH_LEVEL);

  mStartImage     = gBS->StartImage;
  gBS->StartImage = InternalStartImage;

  UPDATE_EFI_TABLE_HEADER_CRC32 (gBS->Hdr);

  EfiRestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
AppleBooterNotifyUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_TPL OldTpl;

  DEBUG_CODE (
    ASSERT (mStartImage != NULL);
    );

  OldTpl = EfiRaiseTPL (TPL_HIGH_LEVEL);

  gBS->StartImage = InternalStartImage;
  mStartImage     = gBS->StartImage;

  UPDATE_EFI_TABLE_HEADER_CRC32 (gBS->Hdr);

  EfiRestoreTPL (OldTpl);

  DEBUG_CODE (
    mStartImage = NULL;
    );

  return EFI_SUCCESS;
}
