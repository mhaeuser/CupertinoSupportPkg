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

#include <Protocol/LoadedImage.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/EfiChecksumLib.h>
#include <Library/VirtualMemoryLib.h>

#include "FirmwareFixes.h"

// RunImageWithOverrides
/** Allocates kernel image reloc block, installs UEFI overrides and starts given image.
    If image returns, then deinstalls overrides and releases kernel image reloc block.

  If started with ImgContext->JumpBuffer, then it will return with LongJump().
**/
EFI_STATUS
RunImageWithOverrides (
  IN     CONST EFI_HANDLE           ImageHandle,
#if !defined (RELOC_BLOCK)
  IN OUT EFI_LOADED_IMAGE_PROTOCOL  *Image,
#endif
  OUT    UINTN                      *ExitDataSize,
  OUT    CHAR16                     **ExitData OPTIONAL
  )
{
  EFI_STATUS Status;

  // save current 64bit state - will be restored later in callback from kernel jump
  // and relocate gAsmCopyAndJumpToKernel32 code to higher mem (for copying kernel back to
  // proper place and jumping back to it)
  Status = PrepareJumpFromKernel ();

  if (!EFI_ERROR (Status)) {
    // init VMem memory pool - will be used after ExitBootServices
    Status = VmAllocateMemoryPool ();

#if defined (RELOC_BLOCK)
    if (!EFI_ERROR (Status)) {
      // allocate block for kernel image relocation
      Status = AllocateRelocationBlock ();
#endif

      if (!EFI_ERROR (Status)) {
        mAllocatePages        = gBS->AllocatePages;
        mGetMemoryMap         = gBS->GetMemoryMap;
        mExitBootServices     = gBS->ExitBootServices;
        mHandleProtocol       = gBS->HandleProtocol;
        gBS->AllocatePages    = MoAllocatePages;
        gBS->GetMemoryMap     = MoGetMemoryMap;
        gBS->ExitBootServices = MoExitBootServices;
        gBS->HandleProtocol   = MoHandleProtocol;

        UPDATE_EFI_TABLE_HEADER_CRC32 (gBS->Hdr);
#if !defined (RELOC_BLOCK)
        mSetVirtualAddressMap     = gRT->SetVirtualAddressMap;
        gRT->SetVirtualAddressMap = MoSetVirtualAddressMap;

        UPDATE_EFI_TABLE_HEADER_CRC32 (gRT->Hdr);

        // force boot.efi to use our copy od system table
        Image->SystemTable    = (EFI_SYSTEM_TABLE *)(UINTN)mSysTableRtArea;
#endif // !defined (RELOC_BLOCK)
        Status                = mStartImage (ImageHandle, ExitDataSize, ExitData);

        // if we get here then boot.efi did not start kernel and we'll try to do some cleanup

        gBS->AllocatePages    = mAllocatePages;
        gBS->GetMemoryMap     = mGetMemoryMap;
        gBS->ExitBootServices = mExitBootServices;
        gBS->HandleProtocol   = mHandleProtocol;

        UPDATE_EFI_TABLE_HEADER_CRC32 (gBS->Hdr);
#if !defined (RELOC_BLOCK)
        gRT->SetVirtualAddressMap = mSetVirtualAddressMap;

        UPDATE_EFI_TABLE_HEADER_CRC32 (gRT->Hdr);
#else
        FreeRelocBlock ();
      }
#endif // defined (RELOC_BLOCK)
    }
  }

  return Status;
}