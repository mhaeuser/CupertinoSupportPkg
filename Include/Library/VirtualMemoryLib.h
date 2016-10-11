/** @file
  x64 Long Mode Virtual Memory Management Definitions  

  References:
    1) IA-32 Intel(R) Architecture Software Developer's Manual Volume 1:Basic Architecture, Intel
    2) IA-32 Intel(R) Architecture Software Developer's Manual Volume 2:Instruction Set Reference, Intel
    3) IA-32 Intel(R) Architecture Software Developer's Manual Volume 3:System Programmer's Guide, Intel
    4) AMD64 Architecture Programmer's Manual Volume 2: System Programming

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.
  Copyright (C) 2012 Damir Mažar.  All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef VIRTUAL_MEMORY_LIB_H_
#define VIRTUAL_MEMORY_LIB_H_

// SYS_CODE64_SEL
#define SYS_CODE64_SEL  0x38

#define VA_FIX_SIGN_EXTEND(VA)  \
  (VA).Page4Kb.SignExtend = (BIT_SET ((VA).Page4Kb.PageMapLevel4Offset, BIT8) ? MAX_UINT16 : 0);

// 64 bit
#define CR3_ADDRESS_MASK  0x000FFFFFFFFFF000
#define CR3_FLAG_PWT      0x0000000000000008
#define CR3_FLAG_PCD      0x0000000000000010

#define PAGE_TABLE_ADDRESS_MASK_4KB  0x000FFFFFFFFFF000
#define PAGE_TABLE_ADDRESS_MASK_2MB  0x000FFFFFFFE00000
#define PAGE_TABLE_ADDRESS_MASK_1GB	 0x000FFFFFC0000000

#pragma pack (1)

// PAGE_MAP_AND_DIRECTORY_PTR
/// Page-Map Level-4 Offset (PML4) and
/// Page-Directory-Pointer Offset (PDPE) entries 4K & 2MB
typedef PACKED union {
  PACKED struct {
    UINT64 Present              : 1;   ///< 0 = Not present in memory, 1 = Present in memory
    UINT64 ReadWrite            : 1;   ///< 0 = Read-Only, 1= Read/Write
    UINT64 UserSupervisor       : 1;   ///< 0 = Supervisor, 1=User
    UINT64 WriteThrough         : 1;   ///< 0 = Write-Back caching, 1=Write-Through caching
    UINT64 CacheDisabled        : 1;   ///< 0 = Cached, 1=Non-Cached
    UINT64 Accessed             : 1;   ///< 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64 Reserved             : 1;   ///< Reserved
    UINT64 Fixed0               : 2;   ///< Must Be Zero
    UINT64 Available            : 3;   ///< Available for use by system software
    UINT64 PageTableBaseAddress : 40;  ///< Page Table Base Address
    UINT64 AvabilableHigh       : 11;  ///< Available for use by system software
    UINT64 NoExecute            : 1;   ///< No Execute bit
  }      Bits;
  UINT64 PackedValue;
} volatile PAGE_MAP_AND_DIRECTORY_PTR;

// PAGE_TABLE_4KB_ENTRY
/// Page Table Entry 4KB
typedef PACKED union {
  PACKED struct {
    UINT64 Present              : 1;   ///< 0 = Not present in memory, 1 = Present in memory
    UINT64 ReadWrite            : 1;   ///< 0 = Read-Only, 1= Read/Write
    UINT64 UserSupervisor       : 1;   ///< 0 = Supervisor, 1=User
    UINT64 WriteThrough         : 1;   ///< 0 = Write-Back caching, 1=Write-Through caching
    UINT64 CacheDisabled        : 1;   ///< 0 = Cached, 1=Non-Cached
    UINT64 Accessed             : 1;   ///< 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64 Dirty                : 1;   ///< 0 = Not Dirty, 1 = written by processor on access to page
    UINT64 PAT                  : 1;
    UINT64 Global               : 1;   ///< 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64 Available            : 3;   ///< Available for use by system software
    UINT64 PageTableBaseAddress : 40;  ///< Page Table Base Address
    UINT64 AvabilableHigh       : 11;  ///< Available for use by system software
    UINT64 NoExecute            : 1;   ///< 0 = Execute Code, 1 = No Code Execution
  }      Bits;
  UINT64 PackedValue;
} volatile PAGE_TABLE_4KB_ENTRY;

// PAGE_TABLE_2MB_ENTRY
/// Page Table Entry 2MB
typedef PACKED union {
  PACKED struct {
    UINT64 Present              : 1;   /// 0 = Not present in memory, 1 = Present in memory
    UINT64 ReadWrite            : 1;   /// 0 = Read-Only, 1= Read/Write
    UINT64 UserSupervisor       : 1;   /// 0 = Supervisor, 1=User
    UINT64 WriteThrough         : 1;   /// 0 = Write-Back caching, 1=Write-Through caching
    UINT64 CacheDisabled        : 1;   /// 0 = Cached, 1=Non-Cached
    UINT64 Accessed             : 1;   /// 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64 Dirty                : 1;   /// 0 = Not Dirty, 1 = written by processor on access to page
    UINT64 Fixed1               : 1;   /// Must be 1 
    UINT64 Global               : 1;   /// 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64 Available            : 3;   /// Available for use by system software
    UINT64 Pat                  : 1;
    UINT64 Fixed0               : 8;   /// Must be zero;
    UINT64 PageTableBaseAddress : 31;  /// Page Table Base Address
    UINT64 AvabilableHigh       : 11;  /// Available for use by system software
    UINT64 NoExecute            : 1;   /// 0 = Execute Code, 1 = No Code Execution
  }      Bits;
  UINT64 PackedValue;
} volatile PAGE_TABLE_2MB_ENTRY;

// PAGE_TABLE_1GB_ENTRY
/// Page Table Entry 1GB
typedef PACKED union {
  PACKED struct {
    UINT64 Present              : 1;   /// 0 = Not present in memory, 1 = Present in memory
    UINT64 ReadWrite            : 1;   /// 0 = Read-Only, 1= Read/Write
    UINT64 UserSupervisor       : 1;   /// 0 = Supervisor, 1=User
    UINT64 WriteThrough         : 1;   /// 0 = Write-Back caching, 1=Write-Through caching
    UINT64 CacheDisabled        : 1;   /// 0 = Cached, 1=Non-Cached
    UINT64 Accessed             : 1;   /// 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64 Dirty                : 1;   /// 0 = Not Dirty, 1 = written by processor on access to page
    UINT64 Fixed1               : 1;   /// Must be 1 
    UINT64 Global               : 1;   /// 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64 Available            : 3;   /// Available for use by system software
    UINT64 Par                  : 1;
    UINT64 Fixed0               : 17;  /// Must be zero;
    UINT64 PageTableBaseAddress : 22;  /// Page Table Base Address
    UINT64 AvabilableHigh       : 11;  /// Available for use by system software
    UINT64 NoExecute            : 1;   /// 0 = Execute Code, 1 = No Code Execution
  }      Bits;
  UINT64 PackedValue;
} volatile PAGE_TABLE_1GB_ENTRY;

// _PAGE_TABLE_ENTRY_PTR
typedef PACKED union {
  PAGE_TABLE_4KB_ENTRY *Entry4Kb;
  PAGE_TABLE_2MB_ENTRY *Entry2Mb;
  PAGE_TABLE_1GB_ENTRY *Entry1Gb;
} PAGE_TABLE_ENTRY_PTR;

// _EFI_VIRTUAL_PAGE
typedef PACKED union {
  PACKED struct {
    UINT64 PhysicalPageOffset         : 12;  /// 0 = Physical Page Offset
    UINT64 PageTableOffset            : 9;   /// 0 = Page Table Offset
    UINT64 PageDirectoryOffset        : 9;   /// 0 = Page Directory Offset
    UINT64 PageDirectoryPointerOffset : 9;   /// 0 = Page Directory Pointer Offset
    UINT64 PageMapLevel4Offset        : 9;   /// 0 = Page Map Level 4 Offset
    UINT64 SignExtend                 : 16;  /// 0 = Sign Extend
  }                   Page4Kb;

  PACKED struct {
    UINT64 PhysicalPageOffset         : 21;  /// 0 = Physical Page Offset
    UINT64 PageDirectoryOffset        : 9;   /// 0 = Page Directory Offset
    UINT64 PageDirectoryPointerOffset : 9;   /// 0 = Page Directory Pointer Offset
    UINT64 PageMapLevel4Offset        : 9;   /// 0 = Page Map Level 4 Offset
    UINT64 SignExtend                 : 16;  /// 0 = Sign Extend
  }                   Page2Mb;

  PACKED struct {
    UINT64 PhysicalPageOffset         : 30;  /// 0 = Physical Page Offset
    UINT64 PageDirectoryPointerOffset : 9;   /// 0 = Page Directory Pointer Offset
    UINT64 PageMapLevel4Offset        : 9;   /// 0 = Page Map Level 4 Offset
    UINT64 SignExtend                 : 16;  /// 0 = Sign Extend
  }                   Page1Gb;
  EFI_VIRTUAL_ADDRESS Address;
} volatile EFI_VIRTUAL_PAGE;

#pragma pack ()

// GetCurrentPageTable
/// Returns pointer to PML4 table in PageTable and PWT and PCD flags in Flags.
// GetCurrentPageTable
PAGE_MAP_AND_DIRECTORY_PTR *
GetCurrentPageTable (
  OUT UINTN  *Flags OPTIONAL
  );

// GetPhysicalAddress
/// Returns physical addr for given virtual addr.
EFI_STATUS
GetPhysicalAddress (
  IN  PAGE_MAP_AND_DIRECTORY_PTR  *PageTable,
  IN  EFI_VIRTUAL_ADDRESS         VirtualAddress,
  OUT EFI_PHYSICAL_ADDRESS        *PhysicalAddress
  );

// VmAllocateMemoryPool
/// Inits vm memory pool. Should be called while boot services are still usable.
EFI_STATUS
VmAllocateMemoryPool (
  VOID
  );

// VmMapVirtualPage
/// Maps (remaps) 4K page given by VirtualAddr to PhysicalAddr page in PageTable.
EFI_STATUS
VmMapVirtualPage (
  IN PAGE_MAP_AND_DIRECTORY_PTR  *PageTable,
  IN EFI_VIRTUAL_ADDRESS         VirtualAddress,
  IN EFI_PHYSICAL_ADDRESS        PhysicalAddress
  );

// VmMapVirtualPages
/// Maps (remaps) NumPages 4K pages given by VirtualAddr to PhysicalAddr pages in PageTable.
EFI_STATUS
VmMapVirtualPages (
  IN PAGE_MAP_AND_DIRECTORY_PTR  *PageTable,
  IN EFI_VIRTUAL_ADDRESS         VirtualAddress,
  IN UINTN                       NoPages,
  IN EFI_PHYSICAL_ADDRESS        PhysicalAddress
  );

// VmFlashCaches
/// Flashes TLB caches.
VOID
VmFlashCaches (
  VOID
  );

#endif // VIRTUAL_MEMORY_LIB_H_
