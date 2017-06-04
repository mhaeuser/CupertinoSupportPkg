/** @file
  Copyright (C) 2012 - 2014 Damir Mažar.  All rights reserved.<BR>
  Portions Copyright (C) 2015 - 2016, CupertinoNet.  All rights reserved.<BR>

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

#include <XnuHibernate.h>

#include <Library/DebugLib.h>

// FixHibernateWake
VOID
FixHibernateWake (
  IN UINTN  ImageHeaderPage
  )
{
  IO_HIBERNATE_IMAGE_HEADER *ImageHeader;
  IO_HIBERNATE_HANDOFF      *Handoff;

  ASSERT (ImageHeaderPage != 0);

  // dmazar:
  // at this stage HIB section is not yet copied from sleep image to it's
  // proper memory destination. so we'll patch entry point in sleep image.
  // we need to remove memory map handoff. my system restarts if we leave it there
  // if mem map handoff is not present, then kernel will not map those new rt pages
  // and that is what we need on our faulty UEFIs.
  // it's the equivalent to RemoveRTFlagMappings() in normal boot.

  // TODO: Check signature

  ImageHeader = (IO_HIBERNATE_IMAGE_HEADER *)(ImageHeaderPage << EFI_PAGE_SHIFT);
  Handoff     = (IO_HIBERNATE_HANDOFF *)(UINTN)(ImageHeader->HandoffPages << EFI_PAGE_SHIFT);
  //ImageHeader->systemTableOffset = (UINT32)(UINTN)(gRelocatedSysTableRtArea - ImageHeader->runtimePages);

  while (Handoff->Type != IoHibernateHandoffTypeEnd) {
    if (Handoff->Type == IoHibernateHandoffTypeMemoryMap) {
      Handoff->Type = (IO_HIBERNATE_HANDOFF_PREFIX | 0xFFFF);

      break;
    }

    Handoff = NEXT_IO_HIBERNATE_HANDOFF (Handoff);
  }
}
