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

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MiscMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/VirtualMemoryLib.h>

#include "../VirtualMemoryInternal.h"

// SYS_CODE64_SEL
#define SYS_CODE64_SEL  0x38

// VA_FIX_SIGN_EXTEND
#define VA_FIX_SIGN_EXTEND(VA)  \
  (VA).Page4Kb.SignExtend = ((((VA).Page4Kb.MapLevel4Offset & BIT8) != 0) ? MAX_UINT16 : 0);

#define CR3_ADDRESS_MASK  0x000FFFFFFFFFF000
#define CR3_FLAG_PWT      0x0000000000000008
#define CR3_FLAG_PCD      0x0000000000000010

#define PAGE_TABLE_ADDRESS_MASK  0x000FFFFFFFFFFFFF
#define PAGE_TABLE_MASK_4KB     (PAGE_TABLE_ADDRESS_MASK & ~(BASE_4KB - 1))
#define PAGE_TABLE_MASK_2MB     (PAGE_TABLE_ADDRESS_MASK & ~(BASE_2MB - 1))
#define PAGE_TABLE_MASK_1GB	    (PAGE_TABLE_ADDRESS_MASK & ~(BASE_1GB - 1))

#pragma pack (1)

// PAGE_MAP_AND_DIRECTORY_PTR
typedef PACKED volatile union {
  PACKED volatile struct {
    UINT64 Present          : 1;   ///< 0 = Not present in memory, 1 = Present in memory
    UINT64 ReadWrite        : 1;   ///< 0 = Read-Only, 1= Read/Write
    UINT64 UserSupervisor   : 1;   ///< 0 = Supervisor, 1=User
    UINT64 WriteThrough     : 1;   ///< 0 = Write-Back caching, 1=Write-Through caching
    UINT64 CacheDisabled    : 1;   ///< 0 = Cached, 1=Non-Cached
    UINT64 Accessed         : 1;   ///< 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64 Reserved         : 1;   ///< Reserved
    UINT64 Fixed0           : 2;   ///< Must Be Zero
    UINT64 Available        : 3;   ///< Available for use by system software
    UINT64 PageTableAddress : 40;  ///< Page Table Base Address
    UINT64 AvabilableHigh   : 11;  ///< Available for use by system software
    UINT64 NoExecute        : 1;   ///< No Execute bit
  }      Bits;
  UINT64 PackedValue;
} PAGE_TABLE_BASE;

// PAGE_TABLE
typedef PAGE_TABLE_BASE PAGE_TABLE;

// PAGE_MAP
typedef PAGE_TABLE_BASE PAGE_MAP;

// PAGE_DIRECTORY_PTR
typedef PAGE_TABLE_BASE PAGE_DIRECTORY_PTR;

// PAGE_DIRECTORY
typedef PAGE_TABLE_BASE PAGE_DIRECTORY;

// PAGE_TABLE_4KB_ENTRY
typedef PACKED volatile union {
  PACKED volatile struct {
    UINT64 Present          : 1;   ///< 0 = Not present in memory, 1 = Present in memory
    UINT64 ReadWrite        : 1;   ///< 0 = Read-Only, 1= Read/Write
    UINT64 UserSupervisor   : 1;   ///< 0 = Supervisor, 1=User
    UINT64 WriteThrough     : 1;   ///< 0 = Write-Back caching, 1=Write-Through caching
    UINT64 CacheDisabled    : 1;   ///< 0 = Cached, 1=Non-Cached
    UINT64 Accessed         : 1;   ///< 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64 Dirty            : 1;   ///< 0 = Not Dirty, 1 = written by processor on access to page
    UINT64 PAT              : 1;
    UINT64 Global           : 1;   ///< 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64 Available        : 3;   ///< Available for use by system software
    UINT64 PageTableAddress : 40;  ///< Page Table Base Address
    UINT64 AvabilableHigh   : 11;  ///< Available for use by system software
    UINT64 NoExecute        : 1;   ///< 0 = Execute Code, 1 = No Code Execution
  }      Bits;
  UINT64 PackedValue;
} PAGE_TABLE_4KB_ENTRY;

// PAGE_TABLE_2MB_ENTRY
typedef PACKED volatile union {
  PACKED volatile struct {
    UINT64 Present          : 1;   /// 0 = Not present in memory, 1 = Present in memory
    UINT64 ReadWrite        : 1;   /// 0 = Read-Only, 1= Read/Write
    UINT64 UserSupervisor   : 1;   /// 0 = Supervisor, 1=User
    UINT64 WriteThrough     : 1;   /// 0 = Write-Back caching, 1=Write-Through caching
    UINT64 CacheDisabled    : 1;   /// 0 = Cached, 1=Non-Cached
    UINT64 Accessed         : 1;   /// 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64 Dirty            : 1;   /// 0 = Not Dirty, 1 = written by processor on access to page
    UINT64 Fixed1           : 1;   /// Must be 1 
    UINT64 Global           : 1;   /// 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64 Available        : 3;   /// Available for use by system software
    UINT64 Pat              : 1;
    UINT64 Fixed0           : 8;   /// Must be zero;
    UINT64 PageTableAddress : 31;  /// Page Table Base Address
    UINT64 AvabilableHigh   : 11;  /// Available for use by system software
    UINT64 NoExecute        : 1;   /// 0 = Execute Code, 1 = No Code Execution
  }      Bits;
  UINT64 PackedValue;
} PAGE_TABLE_2MB_ENTRY;

// PAGE_TABLE_1GB_ENTRY
typedef PACKED volatile union {
  PACKED volatile struct {
    UINT64 Present          : 1;   /// 0 = Not present in memory, 1 = Present in memory
    UINT64 ReadWrite        : 1;   /// 0 = Read-Only, 1= Read/Write
    UINT64 UserSupervisor   : 1;   /// 0 = Supervisor, 1=User
    UINT64 WriteThrough     : 1;   /// 0 = Write-Back caching, 1=Write-Through caching
    UINT64 CacheDisabled    : 1;   /// 0 = Cached, 1=Non-Cached
    UINT64 Accessed         : 1;   /// 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64 Dirty            : 1;   /// 0 = Not Dirty, 1 = written by processor on access to page
    UINT64 Fixed1           : 1;   /// Must be 1 
    UINT64 Global           : 1;   /// 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64 Available        : 3;   /// Available for use by system software
    UINT64 Par              : 1;
    UINT64 Fixed0           : 17;  /// Must be zero;
    UINT64 PageTableAddress : 22;  /// Page Table Base Address
    UINT64 AvabilableHigh   : 11;  /// Available for use by system software
    UINT64 NoExecute        : 1;   /// 0 = Execute Code, 1 = No Code Execution
  }      Bits;
  UINT64 PackedValue;
} PAGE_TABLE_1GB_ENTRY;

// PAGE_TABLE_ENTRY_PTR
typedef PACKED union {
  PAGE_TABLE_4KB_ENTRY *Entry4Kb;
  PAGE_TABLE_2MB_ENTRY *Entry2Mb;
  PAGE_TABLE_1GB_ENTRY *Entry1Gb;
} PAGE_TABLE_ENTRY_PTR;

// EFI_VIRTUAL_PAGE
typedef PACKED volatile union {
  PACKED volatile struct {
    UINT64 PhysicalOffset         : 12;  /// 0 = Physical Page Offset
    UINT64 TableOffset            : 9;   /// 0 = Page Table Offset
    UINT64 DirectoryOffset        : 9;   /// 0 = Page Directory Offset
    UINT64 DirectoryPointerOffset : 9;   /// 0 = Page Directory Pointer Offset
    UINT64 MapLevel4Offset        : 9;   /// 0 = Page Map Level 4 Offset
    UINT64 SignExtend             : 16;  /// 0 = Sign Extend
  }                   Page4Kb;

  PACKED volatile struct {
    UINT64 PhysicalOffset         : 21;  /// 0 = Physical Page Offset
    UINT64 DirectoryOffset        : 9;   /// 0 = Page Directory Offset
    UINT64 DirectoryPointerOffset : 9;   /// 0 = Page Directory Pointer Offset
    UINT64 MapLevel4Offset        : 9;   /// 0 = Page Map Level 4 Offset
    UINT64 SignExtend             : 16;  /// 0 = Sign Extend
  }                   Page2Mb;

  PACKED volatile struct {
    UINT64 PhysicalOffset         : 30;  /// 0 = Physical Page Offset
    UINT64 DirectoryPointerOffset : 9;   /// 0 = Page Directory Pointer Offset
    UINT64 MapLevel4Offset        : 9;   /// 0 = Page Map Level 4 Offset
    UINT64 SignExtend             : 16;  /// 0 = Sign Extend
  }                   Page1Gb;
  EFI_VIRTUAL_ADDRESS Address;
} EFI_VIRTUAL_PAGE;

#pragma pack ()

// VirtualMemoryGetPageTable
VOID *
VirtualMemoryGetPageTable (
  IN OUT UINTN  *Flags OPTIONAL
  )
{
  UINTN Cr3;

  Cr3 = AsmReadCr3 ();

  if (Flags != NULL) {
    *Flags = (Cr3 & (CR3_FLAG_PWT | CR3_FLAG_PCD));
  }

  return (VOID *)(Cr3 & CR3_ADDRESS_MASK);
}

// VirtualMemoryGetPhysicalAddress
EFI_PHYSICAL_ADDRESS
VirtualMemoryGetPhysicalAddress (
  IN VOID                 *PageTable,
  IN EFI_VIRTUAL_ADDRESS  VirtualAddress
  )
{
  EFI_PHYSICAL_ADDRESS PhysicalAddress;

  UINT64               Address;
  EFI_VIRTUAL_PAGE     Page;
  EFI_VIRTUAL_PAGE     Start;
  EFI_VIRTUAL_PAGE     End;
  PAGE_MAP             *MapLevel4;
  PAGE_DIRECTORY_PTR   *DirectoryPtr;
  PAGE_DIRECTORY       *Directory;
  PAGE_TABLE_ENTRY_PTR Table;

  ASSERT (PageTable != NULL);
  ASSERT (VirtualAddress != 0);

  PhysicalAddress = 0;

  Page.Address = VirtualAddress;

  Start.Address                 = 0;
  Start.Page4Kb.MapLevel4Offset = Page.Page4Kb.MapLevel4Offset;

  VA_FIX_SIGN_EXTEND (Start);

  End.Address                 = MAX_UINT64;
  End.Page4Kb.MapLevel4Offset = Page.Page4Kb.MapLevel4Offset;

  VA_FIX_SIGN_EXTEND (End);

  MapLevel4 = (PAGE_MAP *)((PAGE_TABLE *)PageTable + Page.Page4Kb.MapLevel4Offset);
  
  if (MapLevel4->Bits.Present == 1) {
    Address      = (MapLevel4->PackedValue & PAGE_TABLE_MASK_4KB);
    DirectoryPtr = ((PAGE_DIRECTORY_PTR *)Address + Page.Page4Kb.DirectoryPointerOffset);
    
    Start.Page4Kb.DirectoryPointerOffset = Page.Page4Kb.DirectoryPointerOffset;
    End.Page4Kb.DirectoryPointerOffset   = Page.Page4Kb.DirectoryPointerOffset;

    if (DirectoryPtr->Bits.Present == 1) {
      if ((DirectoryPtr->Bits.Fixed0 & BIT1) != 0) {
        Table.Entry1Gb = (PAGE_TABLE_1GB_ENTRY *)DirectoryPtr;
        Address        = (Table.Entry1Gb->PackedValue & PAGE_TABLE_MASK_1GB);

        PhysicalAddress = (Address + Page.Page1Gb.PhysicalOffset);
      } else {
        Address   = (DirectoryPtr->PackedValue & PAGE_TABLE_MASK_4KB);
        Directory = ((PAGE_DIRECTORY *)Address + Page.Page4Kb.DirectoryOffset);

        Start.Page4Kb.DirectoryOffset = Page.Page4Kb.DirectoryOffset;
        End.Page4Kb.DirectoryOffset   = Page.Page4Kb.DirectoryOffset;

        if (Directory->Bits.Present == 1) {
          if ((Directory->Bits.Fixed0, BIT1) != 0) {
            Table.Entry2Mb = (PAGE_TABLE_2MB_ENTRY *)Directory;
            Address        = (Table.Entry2Mb->PackedValue & PAGE_TABLE_MASK_2MB);

            PhysicalAddress = (Address + Page.Page2Mb.PhysicalOffset);
          } else {
            Address = (Directory->PackedValue & PAGE_TABLE_MASK_4KB);
            Table.Entry4Kb = ((PAGE_TABLE_4KB_ENTRY *)Address + Page.Page4Kb.TableOffset);

            Start.Page4Kb.TableOffset = Page.Page4Kb.TableOffset;
            End.Page4Kb.TableOffset   = Page.Page4Kb.TableOffset;

            if (Table.Entry4Kb->Bits.Present == 1) {
              Address = (Table.Entry4Kb->PackedValue & PAGE_TABLE_MASK_4KB);

              PhysicalAddress = (Address + Page.Page4Kb.PhysicalOffset);
            }
          }
        }
      }
    }
  }

  return PhysicalAddress;
}

// VmInternalMapVirtualPage
BOOLEAN
VmInternalMapVirtualPage (
  IN VOID                  *PageTable,
  IN EFI_VIRTUAL_ADDRESS   VirtualAddress,
  IN EFI_PHYSICAL_ADDRESS  PhysicalAddress
  )
{
  BOOLEAN              Result;

  UINT64               Address;
  EFI_VIRTUAL_PAGE     Page;
  EFI_VIRTUAL_PAGE     Start;
  EFI_VIRTUAL_PAGE     End;
  PAGE_MAP             *MapLevel4;
  PAGE_DIRECTORY_PTR   *DirectoryPtr;
  PAGE_DIRECTORY       *Directory;
  PAGE_TABLE_ENTRY_PTR Table;
  PAGE_TABLE_4KB_ENTRY *Entry;
  UINTN                Index;

  Result = FALSE;

  Page.Address = VirtualAddress;
  MapLevel4    = (PAGE_MAP *)((PAGE_TABLE *)PageTable + Page.Page4Kb.MapLevel4Offset);

  // dmazar: There is a problem if our MapLevel4 points to the same table as
  // the first MapLevel4 entry, since we may mess the mapping of first virtual
  // region (happens in VirtualBox and likely DUET).  Check for this on first
  // call and if true, just clear our MapLevel4 - It will be rebuilt later.
  if ((Page.Page4Kb.MapLevel4Offset != 0)
   && (MapLevel4->Bits.Present == 1)
   && (((PAGE_TABLE *)PageTable)->Bits.PageTableAddress == MapLevel4->Bits.PageTableAddress)) {
    MapLevel4->PackedValue = 0;
  }

  Start.Address                 = 0;
  Start.Page4Kb.MapLevel4Offset = Page.Page4Kb.MapLevel4Offset;

  VA_FIX_SIGN_EXTEND (Start);

  End.Address                 = MAX_UINT64;
  End.Page4Kb.MapLevel4Offset = Page.Page4Kb.MapLevel4Offset;

  VA_FIX_SIGN_EXTEND (End);

  if (MapLevel4->Bits.Present == 0) {
    DirectoryPtr = VmInternalAllocatePages (1);

    if (DirectoryPtr == NULL) {
      goto Done;
    }

    ZeroMem ((VOID *)DirectoryPtr, EFI_PAGE_SIZE);

    // dmazar: Initialize this whole 512GB region with 512 1GB entry pages to
    // map first 512GB physical space.
    Address        = 0;
    Table.Entry1Gb = (PAGE_TABLE_1GB_ENTRY *)DirectoryPtr;

    for (Index = 0; Index < 512; ++Index) {
      Table.Entry1Gb->PackedValue    = (Address & PAGE_TABLE_MASK_1GB);
      Table.Entry1Gb->Bits.ReadWrite = 1;
      Table.Entry1Gb->Bits.Present   = 1;
      Table.Entry1Gb->Bits.Fixed1    = 1;

      Address += BASE_1GB;

      ++Table.Entry1Gb;
    }

    MapLevel4->PackedValue    = ((UINT64)DirectoryPtr & PAGE_TABLE_MASK_4KB);
    MapLevel4->Bits.ReadWrite = 1;
    MapLevel4->Bits.Present   = 1;
  }

  Address      = (MapLevel4->PackedValue & PAGE_TABLE_MASK_4KB);
  DirectoryPtr = ((PAGE_DIRECTORY_PTR *)Address + Page.Page4Kb.DirectoryPointerOffset);
  
  Start.Page4Kb.DirectoryPointerOffset = Page.Page4Kb.DirectoryPointerOffset;
  End.Page4Kb.DirectoryPointerOffset   = Page.Page4Kb.DirectoryPointerOffset;

  if ((DirectoryPtr->Bits.Present == 0)
   || ((DirectoryPtr->Bits.Fixed0 & BIT1) != 0)) {
    Directory = VmInternalAllocatePages (1);

    if (Directory == NULL) {
      goto Done;
    }

    ZeroMem ((VOID *)Directory, EFI_PAGE_SIZE);

    if ((DirectoryPtr->Bits.Fixed0 & BIT1) != 0) {
      Address        = (DirectoryPtr->PackedValue & PAGE_TABLE_MASK_1GB);
      Table.Entry2Mb = (PAGE_TABLE_2MB_ENTRY *)Directory;

      for (Index = 0; Index < 512; ++Index) {
        Table.Entry2Mb->PackedValue    = (Address & PAGE_TABLE_MASK_2MB);
        Table.Entry2Mb->Bits.ReadWrite = 1;
        Table.Entry2Mb->Bits.Present   = 1;
        Table.Entry2Mb->Bits.Fixed1    = 1;

        Address += BASE_2MB;

        ++Table.Entry2Mb;
      }
    }

    DirectoryPtr->PackedValue    = ((UINT64)Directory & PAGE_TABLE_MASK_4KB);
    DirectoryPtr->Bits.ReadWrite = 1;
    DirectoryPtr->Bits.Present   = 1;
  }

  Address   = (DirectoryPtr->PackedValue & PAGE_TABLE_MASK_4KB);
  Directory = ((PAGE_DIRECTORY *)Address + Page.Page4Kb.DirectoryOffset);

  Start.Page4Kb.DirectoryOffset = Page.Page4Kb.DirectoryOffset;
  End.Page4Kb.DirectoryOffset   = Page.Page4Kb.DirectoryOffset;

  if ((Directory->Bits.Present == 0)
   || ((Directory->Bits.Fixed0 & BIT1) != 0)) {
    Entry = VmInternalAllocatePages (1);

    if (Entry == NULL) {
      goto Done;
    }

    ZeroMem ((VOID *)Entry, EFI_PAGE_SIZE);

    if ((Directory->Bits.Fixed0, BIT1) != 0) {
      // dmazar: It was a 2MB page.  Initialize a new table array to get the
      // same mapping but with 4KB pages.
      Address = (Directory->PackedValue & PAGE_TABLE_MASK_2MB);

       Table.Entry4Kb = Entry;

      for (Index = 0; Index < 512; ++Index) {
        Table.Entry4Kb->PackedValue    = (Address & PAGE_TABLE_MASK_4KB);
        Table.Entry4Kb->Bits.ReadWrite = 1;
        Table.Entry4Kb->Bits.Present   = 1;

        Address += BASE_4KB;

        ++Table.Entry4Kb;
      }
    }

    Directory->PackedValue    = (((UINT64)Entry) & PAGE_TABLE_MASK_4KB);
    Directory->Bits.ReadWrite = 1;
    Directory->Bits.Present   = 1;
  }

  Address = (Directory->PackedValue & PAGE_TABLE_MASK_4KB);
  Entry   = (((PAGE_TABLE_4KB_ENTRY *)Address) + Page.Page4Kb.TableOffset);
  
  Start.Page4Kb.TableOffset = Page.Page4Kb.TableOffset;
  End.Page4Kb.TableOffset   = Page.Page4Kb.TableOffset;

  Entry->PackedValue    = (UINT64)(PhysicalAddress & PAGE_TABLE_MASK_4KB);
  Entry->Bits.ReadWrite = 1;
  Entry->Bits.Present   = 1;

  Result = TRUE;

Done:
  return Result;
}

// VirtualMemoryFlashCaches
VOID
VirtualMemoryFlashCaches (
  VOID
  )
{
  UINTN Cr3;

  Cr3 = AsmReadCr3 ();

  AsmWriteCr3 (Cr3);
}
