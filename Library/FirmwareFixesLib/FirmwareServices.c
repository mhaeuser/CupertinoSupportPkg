/** @file
  Copyright (C) 2012 - 2014 Damir Mazar.  All rights reserved.<BR>
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

#include <AppleHibernate.h>

#include <Guid/AppleNvram.h>

#include <Protocol/GraphicsOutput.h>
#include <Protocol/LoadedImage.h>

#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/EfiProtocolLib.h>
#include <Library/EfiVariableLib.h>

#include "FirmwareFixes.h"

// mStartImage
STATIC EFI_IMAGE_START mStartImage = NULL;

// mAllocatePages
/// placeholders for storing original Boot and RT Services functions
STATIC EFI_ALLOCATE_PAGES mAllocatePages = NULL;

// mGetMemoryMap
STATIC EFI_GET_MEMORY_MAP mGetMemoryMap = NULL;

// mExitBootServices
STATIC EFI_EXIT_BOOT_SERVICES mExitBootServices = NULL;

// mHandleProtocol
STATIC EFI_HANDLE_PROTOCOL mHandleProtocol = NULL;

// MoHandleProtocol
/** gBS->HandleProtocol override:
    Boot.efi requires EfiGraphicsOutputProtocol on ConOutHandle, but it is not present
    there on Aptio 2.0 through 4.0. EfiGraphicsOutputProtocol exists on some other handle.
    If this is the case, we'll intercept that call and return EfiGraphicsOutputProtocol
    from that other handle.
**/
// TODO: ConSplitter
EFI_STATUS
EFIAPI
MoHandleProtocol (
  IN  EFI_HANDLE  Handle,
  IN  EFI_GUID    *Protocol,
  OUT VOID        **Interface
  )
{
  EFI_STATUS Status;

  Status = mHandleProtocol (Handle, Protocol, Interface);

  // special handling if gEfiGraphicsOutputProtocolGuid is requested by boot.efi
  if ((Status == EFI_UNSUPPORTED)
   && (Handle == gST->ConsoleOutHandle)
   && CompareGuid (Protocol, &gEfiGraphicsOutputProtocolGuid)) {
    // find it on some other handle
    Status    = LocateProtocol (Protocol, NULL, Interface);
  }

  return Status;
}

// MoGetMemoryMap
/** gBS->GetMemoryMap override:
    Returns shrinked memory map. I think kernel can handle up to 128 entries.
**/
EFI_STATUS
EFIAPI
MoGetMemoryMap (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  OUT    UINTN                  *MapKey,
  OUT    UINTN                  *DescriptorSize,
  OUT    UINT32                 *DescriptorVersion
  )
{
  EFI_STATUS Status;

  Status = mGetMemoryMap (MemoryMapSize, MemoryMap, MapKey, DescriptorSize, DescriptorVersion);

  if (!EFI_ERROR (Status)) {
    FixMemoryMap (*MemoryMapSize, MemoryMap, *DescriptorSize, *DescriptorVersion);

#if !defined (RELOC_BLOCK)
    ShrinkMemMap (MemoryMapSize, MemoryMap, *DescriptorSize, *DescriptorVersion);
#endif
  }

  return Status;
}

// MoExitBootServices
/** gBS->ExitBootServices override:
    Patches kernel entry point with jump to our KernelEntryPatchJumpBack().
**/
EFI_STATUS
EFIAPI
MoExitBootServices (
  IN EFI_HANDLE  ImageHandle,
  IN UINTN       MapKey
  )
{
  EFI_STATUS                Status;

  IO_HIBERNATE_IMAGE_HEADER *ImageHeader;
  IO_HIBERNATE_HANDOFF      *Handoff;

  // for tests: we can just return EFI_SUCCESS and continue using Print for debug.
  Status = mExitBootServices (ImageHandle, MapKey);

  if (!EFI_ERROR (Status)) {
#if defined (RELOC_BLOCK)
    KernelEntryFromMachOPatchJump ((VOID*)(UINTN)(gRelocBase + 0x200000), 0); // KASLR is not available for AptioFix and thus slide=0
#else
    // we need hibernate image address for wake
    if (gHibernateWake && (mHibernateImageAddress != 0)) {
      ImageHeader = (IO_HIBERNATE_IMAGE_HEADER *)(UINTN)mHibernateImageAddress;
      Status      = KernelEntryPatchJump ((UINT32)(UINTN)&ImageHeader->FileExtentMap[0] + ImageHeader->FileExtentMapSize + ImageHeader->Restore1CodeOffset);
    }
#endif // defined (RELOC_BLOCK)
  }

  return Status;
}

/** Returns file path from FilePath in allocated memory. Mem should be released by caler.*/
CHAR16 *
FileDevicePathToText (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  CHAR16     *OutFilePathText;

  CHAR16     FilePathText[256]; // possible problem: if filepath is bigger
  UINTN      SizeAll;
  UINTN      Index;
  UINTN      Size;

  FilePathText[0] = L'\0';
  SizeAll         = 0;

  for (Index = 0; Index < 4; ++Index) {
    if ((FilePath == NULL) || (DevicePathType (FilePath) == END_DEVICE_PATH_TYPE)) {
      break;
    }

    if ((DevicePathType (FilePath) == MEDIA_DEVICE_PATH)
     && (DevicePathSubType (FilePath) == MEDIA_FILEPATH_DP)) {
      Size = ((DevicePathNodeLength (FilePath) - 4) / 2);

      if ((SizeAll + Size) < 256) {
        if ((SizeAll > 0) && (FilePathText[(SizeAll - 4) / 2] != L'\\')) {
          StrCat (FilePathText, L"\\");
        }

        StrCat (FilePathText, ((FILEPATH_DEVICE_PATH *)FilePath)->PathName);

        SizeAll = StrSize (FilePathText);
      }
    }

    FilePath = NextDevicePathNode (FilePath);
  }

  OutFilePathText = NULL;
  Size            = StrSize (FilePathText);

  if (Size > sizeof (FilePathText[0])) {
    // we are allocating mem here - should be released by caller
    OutFilePathText = AllocatePool (Size);

    if (OutFilePathText != NULL) {
      StrnCpy (OutFilePathText, FilePathText, sizeof (OutFilePathText));
    }
  }

  return OutFilePathText;
}

// MoStartImage
/** gBS->StartImage override:
    Called to start an efi image.

  If this is boot.efi, then run it with our overrides.
**/
EFI_STATUS
EFIAPI
MoStartImage (
  IN  EFI_HANDLE  ImageHandle,
  OUT UINTN       *ExitDataSize,
  OUT CHAR16      **ExitData OPTIONAL
  )
{
  EFI_STATUS                Status;

  EFI_LOADED_IMAGE_PROTOCOL *Image;
  CHAR16                    *FilePathText;
  UINTN                     Size;

  // find out image name from EfiLoadedImageProtocol
  Status = OpenProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&Image, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);

  if (!EFI_ERROR (Status)) {
    FilePathText = FileDevicePathToText (Image->FilePath);
    Status       = CloseProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, gImageHandle, NULL);

    // check if this is boot.efi
    if ((StrStr (FilePathText, L"boot.efi")) != NULL) {
#if defined (RELOC_BLOCK)
      Status = RunImageWithOverrides (ImageHandle, ExitDataSize, ExitData);
#else
      // the presence of the variable means HibernateWake
      // if the wake is canceled then the variable must be deleted
      Size           = 0;
      Status         = GetVariable (L"boot-switch-vars", &gAppleBootVariableGuid, 0, &Size, NULL);
      gHibernateWake = (Status == EFI_BUFFER_TOO_SMALL);

      if (!gHibernateWake) {
        Size           = 0;
        Status         = GetVariable (L"boot-signature", &gAppleBootVariableGuid, 0, &Size, NULL);

        if (Status == EFI_BUFFER_TOO_SMALL) {
          Size           = 0;
          Status         = GetVariable (L"boot-image-key", &gAppleBootVariableGuid, 0, &Size, NULL);
          gHibernateWake = TRUE;
        }
      }

      Status = RunImageWithOverrides (ImageHandle, Image, ExitDataSize, ExitData);
#endif // defined (RELOC_BLOCK)
    } else {
      Status = mStartImage (ImageHandle, ExitDataSize, ExitData);
    }

    if (FilePathText != NULL) {
      FreePool ((VOID *)FilePathText);
    }
  }

  return Status;
}
