/** @file
  Copyright (C) 2015 - 2017, CupertinoNet.  All rights reserved.<BR>

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

#include <Protocol/AppleBootPolicy.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/EfiBootServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

// mLoadImage
STATIC EFI_IMAGE_LOAD mLoadImage = NULL;

// InternalLoadImage
/** Loads an EFI image into memory. Supports Apple bless.

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
  EFI_DEVICE_PATH_PROTOCOL   *RemainingDevicePath;
  EFI_STATUS                 Status;
  EFI_HANDLE                 Device;
  EFI_DEV_PATH_PTR           DevicePathPtr;
  APPLE_BOOT_POLICY_PROTOCOL *AppleBootPolicy;

  ASSERT ((((SourceSize != 0) ? 1 : 0)
         ^ ((SourceBuffer == NULL) ? 1 : 0)) != 0);

  ASSERT (ImageHandle != NULL);

  if (BootPolicy && (SourceBuffer == NULL) && (DevicePath != NULL)) {
    Status = EfiLocateProtocol (
               &gAppleBootPolicyProtocolGuid,
               NULL,
               (VOID **)&AppleBootPolicy
               );

    if (!EFI_ERROR (Status)) {
      RemainingDevicePath = DevicePath;
      Status              = EfiLocateDevicePath (
                              &gEfiSimpleFileSystemProtocolGuid,
                              &RemainingDevicePath,
                              &Device
                              );

      if (!EFI_ERROR (Status)) {
        RemainingDevicePath   = DuplicateDevicePath (RemainingDevicePath);
        DevicePathPtr.DevPath = RemainingDevicePath;

        Status = EFI_NOT_FOUND;

        while (!IsDevicePathEnd ((CONST VOID *)DevicePathPtr.DevPath)) {
          if ((DevicePathType ((CONST VOID *)DevicePathPtr.DevPath) == MEDIA_DEVICE_PATH)
           && (DevicePathSubType ((CONST VOID *)DevicePathPtr.DevPath) == MEDIA_HARDDRIVE_DP)) {
            DevicePathPtr.DevPath = NextDevicePathNode (
                                      (CONST VOID *)DevicePathPtr.DevPath
                                      );

            if (IsDevicePathEnd ((CONST VOID *)DevicePathPtr.DevPath)) {
              Status = AppleBootPolicy->GetBootFile (
                                          Device,
                                          &DevicePathPtr.FilePath
                                          );

              if (!EFI_ERROR (Status)) {
                DevicePath = DevicePathPtr.DevPath;
              }
            }

            break;
          }

          DevicePathPtr.DevPath = NextDevicePathNode (
                                    (CONST VOID *)DevicePathPtr.DevPath
                                    );
        }

        if (EFI_ERROR (Status)) {
          FreePool ((VOID *)RemainingDevicePath);
        }
      }
    }
  }

  return mLoadImage (
           BootPolicy,
           ParentImageHandle,
           DevicePath,
           SourceBuffer,
           SourceSize,
           ImageHandle
           );
}

// BlessUnload
EFI_STATUS
EFIAPI
BlessUnload (
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

// BlessMain
/** 

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.  
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS          The entry point is executed successfully.
  @retval EFI_ALREADY_STARTED  The protocol has already been installed.
**/
EFI_STATUS
EFIAPI
BlessMain (
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
