/** @file
  Copyright (C) 2012 - 2014 Damir Mažar.  All rights reserved.<BR>
  Portions Copyright (C) 2015 - 2017, CupertinoNet.  All rights reserved.<BR>

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

#ifndef FIRMWARE_FIXES_INTERNAL_H_
#define FIRMWARE_FIXES_INTERNAL_H_

#define MEMORY_DESCRIPTOR_PHYSICAL_TOP(MemoryDescriptor)  \
  ((MemoryDescriptor)->PhysicalStart                      \
    + EFI_PAGES_TO_SIZE ((UINTN)((MemoryDescriptor)->NumberOfPages)))

#define RELOCATION_BLOCK_SIGNATURE  SIGNATURE_32 ('R', 'E', 'L', 'B')

typedef struct {
  UINT32     Signature;
  LIST_ENTRY Link;
  UINT32     RelocatedAddress;
  UINT32     Destination;
} RELOCATION_BLOCK;

extern BOOLEAN mXnuPrepareStartSignaledInCurrentBooter;

VOID
InternalOverrideSystemTable (
  IN VOID  *Registration
  );

VOID
InternalFreeSystemTableCopy (
  VOID
  );

VOID
OverrideFirmwareServices (
  VOID
  );

VOID
RestoreFirmwareServices (
  VOID
  );

VOID
FixMemoryMap (
  IN     UINTN                  MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN     UINTN                  DescriptorSize
  );

VOID
ShrinkMemoryMap (
  IN UINTN                  *MemoryMapSize,
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                  DescriptorSize
  );

/**
  Copies RT flagged areas to separate Memory Map, defines virtual to phisycal
  address mapping and calls SetVirtualAddressMap() only with that partial
  Memory Map.

  About partial Memory Map:
  Some UEFIs are converting pointers to virtual addresses even if they do not
  point to regions with RT flag.  This means that those UEFIs are using
  MemoryDescriptor->VirtualStart even for non-RT regions.  Linux had issues
  with this:
  http://git.kernel.org/?p=linux/kernel/git/torvalds/linux-2.6.git;a=commit;h=7cb00b72876ea2451eb79d468da0e8fb9134aa8a
  They are doing it Windows way now - copying RT descriptors to separate Memory
  Map and passing that stripped map to SetVirtualAddressMap().  We'll do the
  same, although it seems that just assigning VirtualStart = PhysicalStart for
  non-RT areas also does the job.

  @param[in] MemoryMapSize   The size in bytes of VirtualMap.
  @param[in] DescriptorSize  The size in bytes of an entry in the VirtualMap.
  @param[in] VirtualMap      An array of memory descriptors which contain new
                             virtual address mapping information for all
                             runtime ranges.

**/
EFI_MEMORY_DESCRIPTOR *
GetPartialVirtualAddressMap (
  IN UINTN                  MemoryMapSize,
  IN UINTN                  DescriptorSize,
  IN EFI_MEMORY_DESCRIPTOR  *VirtualMap
  );

/**
  Adds virtual to phisycal address mappings for RT areas. This is needed since
  SetVirtualAddressMap() does not work on my Aptio without that.
  Probably because some driver has a bug and is trying to access new virtual
  addresses during the change.  Linux and Windows are doing the same thing and
  problem is not visible there.

  @param[in] MemoryMapSize   The size in bytes of VirtualMap.
  @param[in] DescriptorSize  The size in bytes of an entry in the VirtualMap.
  @param[in] VirtualMap      An array of memory descriptors which contain new
                             virtual address mapping information for all
                             runtime ranges.
 
**/
VOID
MapVirtualPages (
  IN UINTN                  MemoryMapSize,
  IN UINTN                  DescriptorSize,
  IN EFI_MEMORY_DESCRIPTOR  *VirtualMap
  );

#endif // FIRMWARE_FIXES_INTERNAL_H_
