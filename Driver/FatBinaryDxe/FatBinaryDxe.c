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

#include <AppleMacEfi.h>
#include <PiDxe.h>

#include <IndustryStandard/AppleMachoImage.h>
#include <IndustryStandard/AppleFatBinaryImage.h>

#include <Library/CupertinoFatBinaryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/EfiBootServicesLib.h>
#include <Library/MiscRuntimeLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

// mLoadImage
STATIC EFI_IMAGE_LOAD mLoadImage;

// InternalLoadImage
/** Loads an EFI image into memory. Supports the Fat Binary format.

  @param[in]   BootPolicy         If TRUE, indicates that the request
                                  originates from the boot manager, and that
                                  the boot manager is attempting to load
                                  FilePath as a boot selection. Ignored if
                                  SourceBuffer is not NULL.
  @param[in]   ParentImageHandle  The caller's image handle.
  @param[in]   DevicePath         The DeviceHandle specific file path from
                                  which the image is loaded.
  @param[in]   SourceBuffer       If not NULL, a pointer to the memory location
                                  containing a copy of the image to be loaded.
  @param[in]   SourceSize         The size in bytes of SourceBuffer. Ignored if
                                  SourceBuffer is NULL.
  @param[out]  ImageHandle        The pointer to the returned image handle that
                                  is created when the  image is successfully
                                  loaded.

  @retval EFI_SUCCESS             Image was loaded into memory correctly.
  @retval EFI_NOT_FOUND           Both SourceBuffer and DevicePath are NULL.
  @retval EFI_INVALID_PARAMETER   One or more parameters are invalid.
  @retval EFI_UNSUPPORTED         The image type is not supported.
  @retval EFI_OUT_OF_RESOURCES    Image was not loaded due to insufficient
                                  resources.
  @retval EFI_LOAD_ERROR          Image was not loaded because the image format
                                  was corrupt or not understood.
  @retval EFI_DEVICE_ERROR        Image was not loaded because the device
                                  returned a read error.
  @retval EFI_ACCESS_DENIED       Image was not loaded because the platform
                                  policy prohibits the image from being loaded.
                                  NULL is returned in *ImageHandle.
  @retval EFI_SECURITY_VIOLATION  Image was loaded and an ImageHandle was
                                  created with a valid
                                  EFI_LOADED_IMAGE_PROTOCOL. However, the
                                  current platform policy specifies that the
                                  image should not be started.
**/
STATIC
EFI_STATUS
EFIAPI
InternalLoadImage (
  IN  BOOLEAN                   BootPolicy,
  IN  EFI_HANDLE                ParentImageHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  VOID                      *SourceBuffer, OPTIONAL
  IN  UINTN                     SourceSize,
  OUT EFI_HANDLE                *ImageHandle
  )
{
  EFI_STATUS Status;

  VOID       *OriginalBuffer;
  UINT32     AuthenticationStatus;

  ASSERT (ParentImageHandle != NULL);
  ASSERT (DevicePath != NULL);
  ASSERT ((((SourceSize != 0) ? 1 : 0)
    ^ ((SourceBuffer == NULL) ? 1 : 0)) != 0);

  ASSERT (ImageHandle != NULL);
  ASSERT (!EfiAtRuntime ());
  ASSERT (EfiGetCurrentTpl () < TPL_CALLBACK);
  ASSERT (mLoadImage != NULL);

  OriginalBuffer = NULL;

  if (SourceBuffer == NULL) {
    SourceBuffer = GetFileBufferByFilePath (
                     BootPolicy,
                     DevicePath,
                     &SourceSize,
                     &AuthenticationStatus
                     );
  }

  if (SourceBuffer != NULL) {
    OriginalBuffer = SourceBuffer;
    SourceBuffer   = ThinFatBinaryEfiForCurrentCpu (SourceBuffer, &SourceSize);
  }

  Status = mLoadImage (
             BootPolicy,
             ParentImageHandle,
             DevicePath,
             SourceBuffer,
             SourceSize,
             ImageHandle
             );

  if (OriginalBuffer != NULL) {
    FreePool (OriginalBuffer);
  }

  return Status;
}

// FatBinaryUnload
EFI_STATUS
EFIAPI
FatBinaryUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_TPL OldTpl;

  DEBUG_CODE (
    ASSERT (mLoadImage != NULL);
    );

  OldTpl = EfiRaiseTPL (TPL_HIGH_LEVEL);

  gBS->LoadImage = mLoadImage;
  
  UPDATE_EFI_TABLE_CRC32 (gBS);

  EfiRestoreTPL (OldTpl);

  DEBUG_CODE (
    mLoadImage = NULL;
    );

  return EFI_SUCCESS;
}

// FatBinaryMain
/**

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.  
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS         The entry point is executed successfully.
  @retval EFI_ALREADY_STARTED The protocol has already been installed.
**/
EFI_STATUS
EFIAPI
FatBinaryMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_TPL OldTpl;

  DEBUG_CODE (
    ASSERT (mLoadImage == NULL);
    );

  OldTpl = EfiRaiseTPL (TPL_HIGH_LEVEL);

  mLoadImage     = gBS->LoadImage;
  gBS->LoadImage = InternalLoadImage;

  UPDATE_EFI_TABLE_CRC32 (gBS);

  EfiRestoreTPL (OldTpl);

  return EFI_SUCCESS;
}
