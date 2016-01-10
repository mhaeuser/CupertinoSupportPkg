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

#include <Library/BaseMemoryLib.h>

#include "AsmFuncs.h"

// gOrigKernelCode
/// Buffer and size for original kernel entry code
UINT8 gOrigKernelCode[32];

// gOrigKernelCodeSize
UINTN gOrigKernelCodeSize = 0;

// KernelEntryPatchJump
/// Patches kernel entry point with jump to JumpFromKernel (AsmFuncsX64). This will then call KernelEntryPatchJumpBack.
EFI_STATUS
KernelEntryPatchJump (
  IN OUT UINT32  KernelEntry
  )
{
  EFI_STATUS Status;

  // Size of EntryPatchCode code
  gOrigKernelCodeSize = ((UINTN)&gAsmEntryPatchCodeEnd - (UINTN)&gAsmEntryPatchCode);
  Status              = EFI_NOT_FOUND;

  if (gOrigKernelCodeSize <= sizeof (gOrigKernelCode)) {
    // Save original kernel entry code
    CopyMem ((VOID *)gOrigKernelCode, (VOID *)(UINTN)KernelEntry, gOrigKernelCodeSize);

    // Copy EntryPatchCode code to kernel entry address
    CopyMem ((VOID *)(UINTN)KernelEntry, (VOID *)&gAsmEntryPatchCode, gOrigKernelCodeSize);

    // pass KernelEntry to assembler funcs
    // this is not needed really, since asm code will determine
    // kernel entry address from the stack
    gAsmKernelEntry = KernelEntry;
    Status          = EFI_SUCCESS;
  }

  return Status;
}

// PrepareJumpFromKernel
/// Saves current 64 bit state and copies gAsmCopyAndJumpToKernel32 function to higher mem
/// (for copying kernel back to proper place and jumping back to it).
EFI_STATUS
PrepareJumpFromKernel (
  VOID
  )
{
  EFI_STATUS           Status;

  EFI_PHYSICAL_ADDRESS HigherMem;
  UINTN                Size;

  Status = EFI_SUCCESS;

  // chek if already prepared
  if (mSysTableRtArea == 0) {
#if !defined (RELOC_BLOCK)
    if (gHibernateWake)
#endif
    {
      // save current 64bit state - will be restored later in callback from kernel jump
      AsmPrepareJumpFromKernel ();

      // allocate higher memory for gAsmCopyAndJumpToKernel code
      HigherMem = (APPLE_XNU_MAX_ADDRESS + 1);
      Size      = ((UINTN)&gAsmCopyAndJumpToKernelEnd - (UINTN)&gAsmCopyAndJumpToKernel);
      Status    = AllocatePagesFromTop (EfiBootServicesCode, EFI_SIZE_TO_PAGES (Size), (VOID *)&HigherMem);

      if (!EFI_ERROR (Status)) {
        // and relocate it to higher mem
        gAsmCopyAndJumpToKernel32Address =
          (UINT64)((UINTN)HigherMem + ((UINTN)&gAsmCopyAndJumpToKernel32 - (UINTN)&gAsmCopyAndJumpToKernel));

        gAsmCopyAndJumpToKernel64Address =
          (UINT64)((UINTN)HigherMem + ((UINTN)&gAsmCopyAndJumpToKernel64 - (UINTN)&gAsmCopyAndJumpToKernel));

        CopyMem ((VOID *)(UINTN)HigherMem, (VOID *)&gAsmCopyAndJumpToKernel, Size);

        goto RelocateSt;
      }

      goto Return;
    }

  RelocateSt:
    // Allocate 1 RT data page for copy of EFI system table for kernel
    mSysTableRtArea = (APPLE_XNU_MAX_ADDRESS + 1);
    Size            = (UINTN)gST->Hdr.HeaderSize;
    Status          = AllocatePagesFromTop (EfiRuntimeServicesData, EFI_SIZE_TO_PAGES (Size), &mSysTableRtArea);

    if (!EFI_ERROR (Status)) {
      // Copy sys table to our location
      CopyMem ((VOID *)(UINTN)mSysTableRtArea, (VOID *)gST, Size);
    }
  }

Return:
  return Status;
}

// KernelEntryPatchJumpBack
/// Callback called when boot.efi jumps to kernel.
UINTN
EFIAPI
KernelEntryPatchJumpBack (
  IN OUT UINTN  BootArgs
  )
{
#if !defined (RELOC_BLOCK)
  BootArgs = FixHibernateWake (BootArgs);
#elif APTIO_FIX
  BootArgs = FixBootingWithRelocBlock (BootArgs);
#endif // APTIO_FIX_2

  return BootArgs;
}

// KernelEntryFromMachOPatchJump
/// Reads kernel entry from Mach-O load command and patches it with jump to AsmJumpFromKernel.
EFI_STATUS
KernelEntryFromMachOPatchJump (
  IN OUT VOID   *MachOImage,
  IN     UINT32 SlideAddr
  )
{
  EFI_STATUS Status;

  UINTN      KernelEntry;

  KernelEntry = MachOGetEntryAddress (MachOImage);
  Status      = EFI_NOT_FOUND;

  if (KernelEntry != 0) {
    KernelEntry += SlideAddr;
    Status       = KernelEntryPatchJump ((UINT32)KernelEntry + SlideAddr);
  }

  return Status;
}
