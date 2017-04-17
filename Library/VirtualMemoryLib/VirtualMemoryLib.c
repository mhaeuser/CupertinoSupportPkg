/** @file
  Virtual memory functions.

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

#include <MiscBase.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MiscMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/VirtualMemoryLib.h>

// VM_MEMORY_POOL_SIZE
#define VM_MEMORY_POOL_SIZE  SIZE_2MB

// mVmMemoryPool
/// Memory allocation for VM map pages that we will create with VmMapVirtualPage.
/// We need to have it preallocated during boot services.
STATIC VOID *mVmMemoryPool = NULL;

// mVmMemoryPoolFreePages
STATIC UINTN mVmMemoryPoolFreePages = 0;

// GetCurrentPageTable
/// Returns pointer to PML4 table in PageTable and PWT and PCD flags in Flags.
PAGE_MAP_AND_DIRECTORY_PTR *
GetCurrentPageTable (
  OUT UINTN  *Flags OPTIONAL
  )
{
  UINTN Cr3;

  ASSERT (Flags != NULL);

  Cr3 = AsmReadCr3 ();

  if (Flags != NULL) {
    *Flags = SELECT_BITS (Cr3, JOIN_BITS (CR3_FLAG_PWT, CR3_FLAG_PCD));
  }

  return (PAGE_MAP_AND_DIRECTORY_PTR *)APPLY_MASK (Cr3, CR3_ADDRESS_MASK);
}

// GetPhysicalAddress
EFI_STATUS
GetPhysicalAddress (
  IN  PAGE_MAP_AND_DIRECTORY_PTR  *PageTable,
  IN  EFI_VIRTUAL_ADDRESS         VirtualAddress,
  OUT EFI_PHYSICAL_ADDRESS        *PhysicalAddress
  )
{
  EFI_STATUS                 Status;

  EFI_PHYSICAL_ADDRESS       Start;
  EFI_VIRTUAL_PAGE           VirtualPage;
  EFI_VIRTUAL_PAGE           VirtualPageStart;
  EFI_VIRTUAL_PAGE           VirtualPageEnd;
  PAGE_MAP_AND_DIRECTORY_PTR *PageDirectoryPointerEntry;
  PAGE_MAP_AND_DIRECTORY_PTR *PageDirectoryEntry;
  PAGE_TABLE_ENTRY_PTR       PageTableEntry;

  ASSERT (PageTable != NULL);
  ASSERT (VirtualAddress != 0);
  ASSERT (PhysicalAddress != NULL);

  Status = EFI_NO_MAPPING;

  VirtualPage.Address = VirtualAddress;
  PageTable          += VirtualPage.Page4Kb.PageMapLevel4Offset;

  VirtualPageStart.Address                     = 0;
  VirtualPageStart.Page4Kb.PageMapLevel4Offset = VirtualPage.Page4Kb.PageMapLevel4Offset;

  VA_FIX_SIGN_EXTEND (VirtualPageStart);

  VirtualPageEnd.Address                     = MAX_UINT64;
  VirtualPageEnd.Page4Kb.PageMapLevel4Offset = VirtualPage.Page4Kb.PageMapLevel4Offset;

  VA_FIX_SIGN_EXTEND (VirtualPageEnd);
  
  if (!PageTable->Bits.Present == 1) {
    PageDirectoryPointerEntry = (((PAGE_MAP_AND_DIRECTORY_PTR *)APPLY_MASK (PageTable->PackedValue, PAGE_TABLE_ADDRESS_MASK_4KB)) + VirtualPage.Page4Kb.PageDirectoryPointerOffset);
    
    VirtualPageStart.Page4Kb.PageDirectoryPointerOffset = VirtualPage.Page4Kb.PageDirectoryPointerOffset;
    VirtualPageEnd.Page4Kb.PageDirectoryPointerOffset   = VirtualPage.Page4Kb.PageDirectoryPointerOffset;

    if (PageDirectoryPointerEntry->Bits.Present == 1) {
      if (BIT_SET (PageDirectoryPointerEntry->Bits.Fixed0, BIT1)) {
        PageTableEntry.Entry1Gb = (PAGE_TABLE_1GB_ENTRY *)PageDirectoryPointerEntry;
        Start                   = APPLY_MASK (PageTableEntry.Entry1Gb->PackedValue, PAGE_TABLE_ADDRESS_MASK_1GB);
        *PhysicalAddress        = (Start + VirtualPage.Page1Gb.PhysicalPageOffset);

        Status = EFI_SUCCESS;
      } else {
        PageDirectoryEntry                           = (((PAGE_MAP_AND_DIRECTORY_PTR *)APPLY_MASK (PageDirectoryPointerEntry->PackedValue, PAGE_TABLE_ADDRESS_MASK_4KB)) + VirtualPage.Page4Kb.PageDirectoryOffset);
        VirtualPageStart.Page4Kb.PageDirectoryOffset = VirtualPage.Page4Kb.PageDirectoryOffset;
        VirtualPageEnd.Page4Kb.PageDirectoryOffset   = VirtualPage.Page4Kb.PageDirectoryOffset;

        if (PageDirectoryEntry->Bits.Present == 1) {
          if (BIT_SET (PageDirectoryEntry->Bits.Fixed0, BIT1)) {
            PageTableEntry.Entry2Mb = (PAGE_TABLE_2MB_ENTRY *)PageDirectoryEntry;
            Start                   = APPLY_MASK (
                                        PageTableEntry.Entry2Mb->PackedValue,
                                        PAGE_TABLE_ADDRESS_MASK_2MB
                                        );

            *PhysicalAddress = (Start + VirtualPage.Page2Mb.PhysicalPageOffset);

            Status = EFI_SUCCESS;
          } else {
            // PageTableEntry
            PageTableEntry.Entry4Kb                  = (((PAGE_TABLE_4KB_ENTRY *)APPLY_MASK (PageDirectoryEntry->PackedValue, PAGE_TABLE_ADDRESS_MASK_4KB)) + VirtualPage.Page4Kb.PageTableOffset);
            VirtualPageStart.Page4Kb.PageTableOffset = VirtualPage.Page4Kb.PageTableOffset;
            VirtualPageEnd.Page4Kb.PageTableOffset   = VirtualPage.Page4Kb.PageTableOffset;

            if (PageTableEntry.Entry4Kb->Bits.Present == 1) {
              Start            = APPLY_MASK (
                                   PageTableEntry.Entry4Kb->PackedValue,
                                   PAGE_TABLE_ADDRESS_MASK_4KB
                                   );

              *PhysicalAddress = (Start + VirtualPage.Page4Kb.PhysicalPageOffset);

              Status = EFI_SUCCESS;
            }
          }
        }
      }
    }
  }

  return Status;;
}

// VmAllocateMemoryPool
/// Inits vm memory pool. Should be called while boot services are still usable.
EFI_STATUS
VmAllocateMemoryPool (
  VOID
  )
{
  EFI_STATUS           Status;

  EFI_PHYSICAL_ADDRESS Address;

  Status = EFI_SUCCESS;

  if (mVmMemoryPool == NULL) {
    mVmMemoryPoolFreePages = EFI_SIZE_TO_PAGES (VM_MEMORY_POOL_SIZE);
    Address                = BASE_4GB;

    mVmMemoryPool = AllocatePagesFromTop (
                      EfiBootServicesData,
                      mVmMemoryPoolFreePages,
                      Address
                      );
  }

  return Status;
}

// VmAllocatePages
/// Central method for allocating pages for VM page maps.
VOID *
VmAllocatePages (
  IN UINTN  NoPages
  )
{
  VOID *AllocatedPages;

  AllocatedPages = NULL;

  if (mVmMemoryPoolFreePages >= NoPages) {
    AllocatedPages          = mVmMemoryPool;
    mVmMemoryPool           = (VOID *)((UINTN)mVmMemoryPool + EFI_PAGES_TO_SIZE (NoPages));
    mVmMemoryPoolFreePages -= NoPages;
  }

  return AllocatedPages;
}

// VmMapVirtualPage
/// Maps (remaps) 4K page given by Address to PhysicalAddress page in PageTable.
EFI_STATUS
VmMapVirtualPage (
  IN PAGE_MAP_AND_DIRECTORY_PTR  *PageTable,
  IN EFI_VIRTUAL_ADDRESS         VirtualAddress,
  IN EFI_PHYSICAL_ADDRESS        PhysicalAddress
  )
{
  EFI_STATUS                 Status;

  EFI_PHYSICAL_ADDRESS       Start;
  EFI_VIRTUAL_PAGE           VirtualPage;
  EFI_VIRTUAL_PAGE           VirtualPageStart;
  EFI_VIRTUAL_PAGE           VirtualPageEnd;
  PAGE_MAP_AND_DIRECTORY_PTR *PageMapLevel4;
  PAGE_MAP_AND_DIRECTORY_PTR *PageDirectoryPointerEntry;
  PAGE_MAP_AND_DIRECTORY_PTR *PageDirectoryEntry;
  PAGE_TABLE_ENTRY_PTR       PageTableEntry;
  PAGE_TABLE_4KB_ENTRY       *PageTable4KbEntry2;
  UINTN                      Index;

  Status = EFI_NO_MAPPING;

  VirtualPage.Address = VirtualAddress;
  PageMapLevel4       = (PageTable + VirtualPage.Page4Kb.PageMapLevel4Offset);

  // there is a problem if our PageMapLevel4 points to the same table as first PageMapLevel4 entry
  // since we may mess the mapping of first virtual region (happens in VBox and probably DUET).
  // check for this on first call and if true, just clear our PageMapLevel4 - we'll rebuild it in later step
  if ((PageMapLevel4 != PageTable)
   && (PageMapLevel4->Bits.Present == 1)
   && (PageTable->Bits.PageTableBaseAddress == PageMapLevel4->Bits.PageTableBaseAddress)) {
    PageMapLevel4->PackedValue = 0;
  }

  VirtualPageStart.Address                     = 0;
  VirtualPageStart.Page4Kb.PageMapLevel4Offset = VirtualPage.Page4Kb.PageMapLevel4Offset;

  VA_FIX_SIGN_EXTEND (VirtualPageStart);

  VirtualPageEnd.Address                     = MAX_UINT64;
  VirtualPageEnd.Page4Kb.PageMapLevel4Offset = VirtualPage.Page4Kb.PageMapLevel4Offset;

  VA_FIX_SIGN_EXTEND (VirtualPageEnd);

  if (PageMapLevel4->Bits.Present == 0) {
    PageDirectoryPointerEntry = VmAllocatePages (1);

    if (PageDirectoryPointerEntry == NULL) {
      goto Done;
    }

    ZeroMem ((VOID *)PageDirectoryPointerEntry, EFI_PAGE_SIZE);

    // init this whole 512GB region with 512 1GB entry pages to map first 512GB phys space
    Start                   = 0;
    PageTableEntry.Entry1Gb = (PAGE_TABLE_1GB_ENTRY *)PageDirectoryPointerEntry;

    for (Index = 0; Index < 512; ++Index) {
      PageTableEntry.Entry1Gb->PackedValue    = APPLY_MASK (Start, PAGE_TABLE_ADDRESS_MASK_1GB);
      PageTableEntry.Entry1Gb->Bits.ReadWrite = 1;
      PageTableEntry.Entry1Gb->Bits.Present   = 1;
      PageTableEntry.Entry1Gb->Bits.Fixed1    = 1;

      Start += SIZE_1GB;

      ++PageTableEntry.Entry1Gb;
    }

    PageMapLevel4->PackedValue    = APPLY_MASK (((UINT64)PageDirectoryPointerEntry), PAGE_TABLE_ADDRESS_MASK_4KB);
    PageMapLevel4->Bits.ReadWrite = 1;
    PageMapLevel4->Bits.Present   = 1;
    // and continue with mapping ...
  }

  PageDirectoryPointerEntry = (((PAGE_MAP_AND_DIRECTORY_PTR *)APPLY_MASK (
                                                                PageMapLevel4->PackedValue,
                                                                PAGE_TABLE_ADDRESS_MASK_4KB
                                                                ))
                                + VirtualPage.Page4Kb.PageDirectoryPointerOffset);
  
  VirtualPageStart.Page4Kb.PageDirectoryPointerOffset = VirtualPage.Page4Kb.PageDirectoryPointerOffset;
  VirtualPageEnd.Page4Kb.PageDirectoryPointerOffset   = VirtualPage.Page4Kb.PageDirectoryPointerOffset;

  if ((PageDirectoryPointerEntry->Bits.Present == 0)
   || BIT_SET (PageDirectoryPointerEntry->Bits.Fixed0, BIT1)) {
    PageDirectoryEntry = VmAllocatePages (1);

    if (PageDirectoryEntry == NULL) {
      goto Done;
    }

    ZeroMem ((VOID *)PageDirectoryEntry, EFI_PAGE_SIZE);

    if (BIT_SET (PageDirectoryPointerEntry->Bits.Fixed0, BIT1)) {
      Start                   = APPLY_MASK (PageDirectoryPointerEntry->PackedValue, PAGE_TABLE_ADDRESS_MASK_1GB);
      PageTableEntry.Entry2Mb = (PAGE_TABLE_2MB_ENTRY *)PageDirectoryEntry;

      for (Index = 0; Index < 512; ++Index) {
        PageTableEntry.Entry2Mb->PackedValue    = APPLY_MASK (Start, PAGE_TABLE_ADDRESS_MASK_2MB);
        PageTableEntry.Entry2Mb->Bits.ReadWrite = 1;
        PageTableEntry.Entry2Mb->Bits.Present   = 1;
        PageTableEntry.Entry2Mb->Bits.Fixed1    = 1;
        Start                                  += SIZE_2MB;
        ++PageTableEntry.Entry2Mb;
      }
    }

    PageDirectoryPointerEntry->PackedValue    = APPLY_MASK ((UINT64)PageDirectoryEntry, PAGE_TABLE_ADDRESS_MASK_4KB);
    PageDirectoryPointerEntry->Bits.ReadWrite = 1;
    PageDirectoryPointerEntry->Bits.Present   = 1;
    // and continue with mapping ...
  }

  PageDirectoryEntry                           = ((PAGE_MAP_AND_DIRECTORY_PTR *)APPLY_MASK (PageDirectoryPointerEntry->PackedValue, PAGE_TABLE_ADDRESS_MASK_4KB) + VirtualPage.Page4Kb.PageDirectoryOffset);
  VirtualPageStart.Page4Kb.PageDirectoryOffset = VirtualPage.Page4Kb.PageDirectoryOffset;
  VirtualPageEnd.Page4Kb.PageDirectoryOffset   = VirtualPage.Page4Kb.PageDirectoryOffset;

  if ((PageDirectoryEntry->Bits.Present == 0) || BIT_SET (PageDirectoryEntry->Bits.Fixed0, BIT1)) {
    PageTableEntry.Entry4Kb = VmAllocatePages (1);

    if (PageTableEntry.Entry4Kb == NULL) {
      goto Done;
    }

    ZeroMem ((VOID *)PageTableEntry.Entry4Kb, EFI_PAGE_SIZE);

    if (BIT_SET (PageDirectoryEntry->Bits.Fixed0, BIT1)) {
      // was 2MB page - init new PageTableEntry array to get the same mapping but with 4KB pages
      Start = APPLY_MASK (PageDirectoryEntry->PackedValue, PAGE_TABLE_ADDRESS_MASK_2MB);

      PageTable4KbEntry2 = PageTableEntry.Entry4Kb;

      for (Index = 0; Index < 512; ++Index) {
        PageTable4KbEntry2->PackedValue    = APPLY_MASK (Start, PAGE_TABLE_ADDRESS_MASK_4KB);
        PageTable4KbEntry2->Bits.ReadWrite = 1;
        PageTable4KbEntry2->Bits.Present   = 1;
        Start                             += SIZE_4KB;

        ++PageTable4KbEntry2;
      }
    }

    PageDirectoryEntry->PackedValue    = APPLY_MASK (((UINT64)PageTableEntry.Entry4Kb), PAGE_TABLE_ADDRESS_MASK_4KB);
    PageDirectoryEntry->Bits.ReadWrite = 1;
    PageDirectoryEntry->Bits.Present   = 1;
    // and continue with mapping ...
  }

  PageTableEntry.Entry4Kb = (((PAGE_TABLE_4KB_ENTRY *)APPLY_MASK (PageDirectoryEntry->PackedValue, PAGE_TABLE_ADDRESS_MASK_4KB)) + VirtualPage.Page4Kb.PageTableOffset);
  
  VirtualPageStart.Page4Kb.PageTableOffset = VirtualPage.Page4Kb.PageTableOffset;
  VirtualPageEnd.Page4Kb.PageTableOffset   = VirtualPage.Page4Kb.PageTableOffset;

  PageTableEntry.Entry4Kb->PackedValue    = APPLY_MASK (PhysicalAddress, PAGE_TABLE_ADDRESS_MASK_4KB);
  PageTableEntry.Entry4Kb->Bits.ReadWrite = 1;
  PageTableEntry.Entry4Kb->Bits.Present   = 1;

  Status = EFI_SUCCESS;

 Done:
  return Status;
}

// VmMapVirtualPages
/// Maps (remaps) NumberOfPages 4K pages given by VirtualAddr to PhysicalAddr pages in PageTable.
EFI_STATUS
VmMapVirtualPages (
  IN PAGE_MAP_AND_DIRECTORY_PTR  *PageTable,
  IN EFI_VIRTUAL_ADDRESS         VirtualAddress,
  IN UINT64                      NumberOfPages,
  IN EFI_PHYSICAL_ADDRESS        PhysicalAddress
  )
{
  EFI_STATUS Status;

  Status = EFI_SUCCESS;

  while ((NumberOfPages > 0) && !EFI_ERROR (Status)) {
    Status = VmMapVirtualPage (PageTable, VirtualAddress, PhysicalAddress);

    VirtualAddress  += EFI_PAGE_SIZE;
    PhysicalAddress += EFI_PAGE_SIZE;

    --NumberOfPages;
  }

  return Status;
}

// VmFlashCaches
/// Flashes TLB caches.
VOID
VmFlashCaches (
  VOID
  )
{
  AsmWriteCr3 (AsmReadCr3 ());
}
