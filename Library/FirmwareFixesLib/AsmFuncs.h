/** @file
  Some assembler helper functions plus boot.efi kernel jump callback

  Copyright (C) 2012 - 2013 Damir Mazar.  All rights reserved.<BR>
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

#ifndef ASM_FUNCS_H_
#define ASM_FUNCS_H_

// PrepareJumpFromKernel
/** Save 64 bit state that will be restored on callback.
**/
VOID
EFIAPI
AsmPrepareJumpFromKernel (
  VOID
  );

// JumpFromKernel
/** Callback function, 32 and 64 bit, that is called when boot.efi jumps to kernel address.
**/
VOID
EFIAPI
AsmJumpFromKernel (
  VOID
  );

// EntryPatchCode
/// Address and end address of the 32 and 64 bit code that is copied to kernel entry address to jump back to our code,
/// to JumpFromKernel().
extern UINT8 gAsmEntryPatchCode;

// gAsmEntryPatchCodeEnd
extern UINT8 gAsmEntryPatchCodeEnd;

// gAsmCopyAndJumpToKernel
/// 32 bit function start and end that copies kernel to proper mem and jumps to kernel.
extern UINT8 gAsmCopyAndJumpToKernel;

// gAsmCopyAndJumpToKernel32
extern UINT8 gAsmCopyAndJumpToKernel32;

// gAsmCopyAndJumpToKernel64
extern UINT8 gAsmCopyAndJumpToKernel64;

// gAsmCopyAndJumpToKernelEnd
extern UINT8 gAsmCopyAndJumpToKernelEnd;

// gAsmKernelEntry
extern UINT64 gAsmKernelEntry;

// gAsmKernelImageStartReloc
extern UINT64 gAsmKernelImageStartReloc;

// gAsmKernelImageStart
extern UINT64 gAsmKernelImageStart;

// gAsmKernelImageSize
extern UINT64 gAsmKernelImageSize;

// gAsmCopyAndJumpToKernel32Address
extern UINT64 gAsmCopyAndJumpToKernel32Address;

// gAsmCopyAndJumpToKernel64Address
extern UINT64 gAsmCopyAndJumpToKernel64Address;

#endif // ASM_FUNCS_H_
