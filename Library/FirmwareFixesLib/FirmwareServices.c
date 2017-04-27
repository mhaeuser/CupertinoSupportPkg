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

#include <Guid/XnuPrepareStartNamedEvent.h>

#include <Protocol/GraphicsOutput.h>
#include <Protocol/LoadedImage.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/EfiBootServicesLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/VirtualMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/XnuSupportMemoryAllocationLib.h>

#include "FirmwareFixesInternal.h"
#include "Library/MiscRuntimeLib.h"

// FIRMWARE_SERVICES_PRIVATE_DATA
typedef struct {
  EFI_ALLOCATE_PAGES          AllocatePages;
  EFI_ALLOCATE_POOL           AllocatePool;
  EFI_EXIT_BOOT_SERVICES      ExitBootServices;
  EFI_FREE_PAGES              FreePages;
  EFI_FREE_POOL               FreePool;
  EFI_GET_MEMORY_MAP          GetMemoryMap;
  EFI_HANDLE_PROTOCOL         HandleProtocol;
  EFI_SET_VIRTUAL_ADDRESS_MAP SetVirtualAddressMap;
} FIRMWARE_SERVICES_PRIVATE_DATA;

// mDisableMemoryAllocationServices
STATIC BOOLEAN mDisableMemoryAllocationServices = FALSE;

// mFirmwareServicesOverriden
STATIC BOOLEAN mFirmwareServicesOverriden = FALSE;

// mPrivateData
STATIC FIRMWARE_SERVICES_PRIVATE_DATA mPrivateData = {
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

// InternalHandleProtocol
/** Queries a handle to determine if it supports a specified protocol.

  @param[in]  Handle     The handle being queried.
  @param[in]  Protocol   The published unique identifier of the protocol.
  @param[out] Interface  Supplies the address where a pointer to the
                         corresponding Protocol Interface is returned.

  @retval EFI_SUCCESS            The interface information for the specified
                                 protocol was returned.
  @retval EFI_UNSUPPORTED        The device does not support the specified
                                 protocol.
  @retval EFI_INVALID_PARAMETER  Handle is NULL.
  @retval EFI_INVALID_PARAMETER  Protocol is NULL.
  @retval EFI_INVALID_PARAMETER  Interface is NULL.
**/
STATIC
EFI_STATUS
EFIAPI
InternalHandleProtocol (
  IN  EFI_HANDLE  Handle,
  IN  EFI_GUID    *Protocol,
  OUT VOID        **Interface
  )
{
  EFI_STATUS Status;

  ASSERT (Handle != NULL);
  ASSERT (Protocol != NULL);
  ASSERT (Interface != NULL);
  ASSERT (!EfiAtRuntime ());
  ASSERT (EfiGetCurrentTpl () <= TPL_NOTIFY);

  Status = mPrivateData.HandleProtocol (Handle, Protocol, Interface);

  if ((Status == EFI_UNSUPPORTED)
   && (Handle == gST->ConsoleOutHandle)
   && CompareGuid (Protocol, &gEfiGraphicsOutputProtocolGuid)) {
    Status = EfiLocateProtocol (Protocol, NULL, Interface);
  }

  return Status;
}

// MoSetVirtualAddressMap
/** Changes the runtime addressing mode of EFI firmware from physical to
    virtual.

  @param[in] MemoryMapSize      The size in bytes of VirtualMap.
  @param[in] DescriptorSize     The size in bytes of an entry in the
                                VirtualMap.
  @param[in] DescriptorVersion  The version of the structure entries in
                                VirtualMap.
  @param[in] VirtualMap         An array of memory descriptors which contain
                                new virtual address mapping information for all
                                runtime ranges.

  @retval EFI_SUCCESS            The virtual address map has been applied.
  @retval EFI_UNSUPPORTED        EFI firmware is not at runtime, or the EFI
                                 firmware is already in virtual address mapped
                                 mode.
  @retval EFI_INVALID_PARAMETER  DescriptorSize or DescriptorVersion is
                                 invalid.
  @retval EFI_NO_MAPPING         A virtual address was not supplied for a range
                                 in the memory map that requires a mapping.
  @retval EFI_NOT_FOUND          A virtual address was supplied for an address
                                 that is not found in the memory map.
**/
STATIC
EFI_STATUS
EFIAPI
MoSetVirtualAddressMap (
  IN UINTN                  MemoryMapSize,
  IN UINTN                  DescriptorSize,
  IN UINT32                 DescriptorVersion,
  IN EFI_MEMORY_DESCRIPTOR  *VirtualMap
  )
{
  ASSERT (MemoryMapSize > 0);
  ASSERT (DescriptorSize > 0);
  ASSERT (DescriptorSize < MemoryMapSize);
  ASSERT ((MemoryMapSize % DescriptorSize) == 0);
  ASSERT (VirtualMap != NULL);

  if (PcdGetBool (PcdPartialVirtualAddressMap)) {
    VirtualMap = GetPartialVirtualAddressMap (
                   MemoryMapSize,
                   DescriptorSize,
                   VirtualMap
                   );
  }

  if (PcdGetBool (PcdMapVirtualPages)) {
    MapVirtualPages (
      MemoryMapSize,
      DescriptorSize,
      VirtualMap
      );
  }

  return mPrivateData.SetVirtualAddressMap (
                             MemoryMapSize,
                             DescriptorSize,
                             DescriptorVersion,
                             VirtualMap
                             );
}

// XnuPrepareStartNotify
/** Invoke a notification event

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  The pointer to the notification function's context, which
                      is implementation-dependent.
**/
STATIC
VOID
EFIAPI
XnuPrepareStartNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  // TODO: Call KernelHookLib
}

// InternalGetMemoryMap
/** Returns the current memory map.

  @param[in, out] MemoryMapSize      A pointer to the size, in bytes, of the
                                     MemoryMap buffer.  On input, this is the
                                     size of the buffer allocated by the
                                     caller.   On output, it is the size of the
                                     buffer returned by the firmware if the
                                     buffer was large enough, or the size of
                                     the buffer needed to contain the map if
                                     the buffer was too small.
  @param[in, out] MemoryMap          A pointer to the buffer in which firmware
                                     places the current memory map.
  @param[out]     MapKey             A pointer to the location in which
                                     firmware returns the key for the current
                                     memory map.
  @param[out]     DescriptorSize     A pointer to the location in which
                                     firmware returns the size, in bytes, of an
                                     individual EFI_MEMORY_DESCRIPTOR.
  @param[out]     DescriptorVersion  A pointer to the location in which
                                     firmware returns the version number
                                     associated with the EFI_MEMORY_DESCRIPTOR.

  @retval EFI_SUCCESS            The memory map was returned in the MemoryMap
                                 buffer.
  @retval EFI_BUFFER_TOO_SMALL   The MemoryMap buffer was too small.  The
                                 current buffer size needed to hold the memory
                                 map is returned in MemoryMapSize.
  @retval EFI_INVALID_PARAMETER  1) MemoryMapSize is NULL.
                                 2) The MemoryMap buffer is not too small and
                                    MemoryMap is NULL.
**/
STATIC
EFI_STATUS
EFIAPI
InternalGetMemoryMap (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  OUT    UINTN                  *MapKey,
  OUT    UINTN                  *DescriptorSize,
  OUT    UINT32                 *DescriptorVersion
  )
{
  STATIC BOOLEAN NonAppleBooterCall = FALSE;

  EFI_STATUS     Status;

  ASSERT (MemoryMapSize != NULL);
  ASSERT ((((*MemoryMapSize > 0) ? 1 : 0)
          ^ ((MemoryMap == NULL) ? 1 : 0)) != 0);

  ASSERT (!EfiAtRuntime ());
  ASSERT (EfiGetCurrentTpl () <= TPL_NOTIFY);

  if (!NonAppleBooterCall) {
    DEBUG_CODE (
      ASSERT (!mXnuPrepareStartSignaledInCurrentBooter);
      );

    NonAppleBooterCall = TRUE;

    EfiNamedEventSignal (&gXnuPrepareStartNamedEventGuid);

    NonAppleBooterCall = FALSE;

    DEBUG_CODE (
      mXnuPrepareStartSignaledInCurrentBooter = TRUE;
      );
  }

  Status = mPrivateData.GetMemoryMap (
                          MemoryMapSize,
                          MemoryMap,
                          MapKey,
                          DescriptorSize,
                          DescriptorVersion
                          );

  if (!NonAppleBooterCall && !EFI_ERROR (Status)) {
    if (PcdGetBool (PcdShrinkMemoryMap)) {
      ShrinkMemoryMap (
        MemoryMapSize,
        MemoryMap,
        *DescriptorSize
        );
    }

    if (PcdGetBool (PcdFixMemoryMap)) {
      FixMemoryMap (
        *MemoryMapSize,
        MemoryMap,
        *DescriptorSize
        );
    }
  }

  return Status;
}

// InternalExitBootServices
/** Terminates all boot services.

  @param[in] ImageHandle  Handle that identifies the exiting image.
  @param[in] MapKey       Key to the latest memory map.

  @retval EFI_SUCCESS            Boot services have been terminated.
  @retval EFI_INVALID_PARAMETER  MapKey is incorrect.

**/
STATIC
EFI_STATUS
EFIAPI
InternalExitBootServices (
  IN EFI_HANDLE  ImageHandle,
  IN UINTN       MapKey
  )
{
  EFI_STATUS Status;

  mDisableMemoryAllocationServices = TRUE;

  Status = mPrivateData.ExitBootServices (ImageHandle, MapKey);

  ASSERT (!EFI_ERROR (Status));

  mDisableMemoryAllocationServices = FALSE;

  return Status;
}

// InternalAllocatePages
/** Allocates memory pages from the system.

  @param[in]      Type        The type of allocation to perform.
  @param[in]      MemoryType  The type of memory to allocate.
                              MemoryType values in the range
                              0x70000000..0x7FFFFFFF are reserved for OEM use.
                              MemoryType values in the range
                              0x80000000..0xFFFFFFFF are reserved for use by
                              UEFI OS loaders that are provided by operating
                              system vendors.
  @param[in]      Pages       The number of contiguous 4 KB pages to allocate.
  @param[in, out] Memory      The pointer to a physical address.  On input, the
                              way in which the address is used depends on the
                              value of Type.

  @retval EFI_SUCCESS            The requested pages were allocated.
  @retval EFI_INVALID_PARAMETER  1) Type is not AllocateAnyPages or
                                 AllocateMaxAddress or AllocateAddress.
                                 2) MemoryType is in the range
                                 EfiMaxMemoryType..0x6FFFFFFF.
                                 3) Memory is NULL.
                                 4) MemoryType is EfiPersistentMemory.
  @retval EFI_OUT_OF_RESOURCES   The pages could not be allocated.
  @retval EFI_NOT_FOUND          The requested pages could not be found.
**/
STATIC
EFI_STATUS
EFIAPI
InternalAllocatePages (
  IN     EFI_ALLOCATE_TYPE     Type,
  IN     EFI_MEMORY_TYPE       MemoryType,
  IN     UINTN                 Pages,
  IN OUT EFI_PHYSICAL_ADDRESS  *Memory
  )
{
  EFI_STATUS Status;

  Status = EFI_OUT_OF_RESOURCES;

  if (!mDisableMemoryAllocationServices) {
    Status = mPrivateData.AllocatePages (Type, MemoryType, Pages, Memory);
  }

  return Status;
}

// InternalFreePages
/** Frees memory pages.

  @param[in] Memory  The base physical address of the pages to be freed.
  @param[in] Pages   The number of contiguous 4 KB pages to free.

  @retval EFI_SUCCESS            The requested pages were freed.
  @retval EFI_INVALID_PARAMETER  Memory is not a page-aligned address or Pages
                                 is invalid.
  @retval EFI_NOT_FOUND          The requested memory pages were not allocated
                                 with AllocatePages().
**/
STATIC
EFI_STATUS
EFIAPI
InternalFreePages (
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 Pages
  )
{
  EFI_STATUS Status;

  Status = EFI_SUCCESS;

  if (!mDisableMemoryAllocationServices) {
    Status = mPrivateData.FreePages (Memory, Pages);
  }

  return Status;
}

// InternalAllocatePool
/** Allocates pool memory.

  @param[in]  PoolType  The type of pool to allocate.  MemoryType values in the
                        range 0x70000000..0x7FFFFFFF are reserved for OEM use.
                        MemoryType values in the range 0x80000000..0xFFFFFFFF
                        are reserved for use by UEFI OS loaders that are
                        provided by operating system vendors.  The number of
                        bytes to allocate from the pool.
  @param[out] Buffer    A pointer to a pointer to the allocated buffer if the
                        call succeeds; undefined otherwise.

  @retval EFI_SUCCESS            The requested number of bytes was allocated.
  @retval EFI_OUT_OF_RESOURCES   The pool requested could not be allocated.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
                                 PoolType is in the range
                                 EfiMaxMemoryType..0x6FFFFFFF.
                                 PoolType is EfiPersistentMemory.
**/
STATIC
EFI_STATUS
EFIAPI
InternalAllocatePool (
  IN  EFI_MEMORY_TYPE  PoolType,
  IN  UINTN            Size,
  OUT VOID             **Buffer
  )
{
  EFI_STATUS Status;

  Status = EFI_OUT_OF_RESOURCES;

  if (!mDisableMemoryAllocationServices) {
    Status = mPrivateData.AllocatePool (PoolType, Size, Buffer);
  }

  return Status;
}

// InternalFreePool
/** Returns pool memory to the system.

  @param[in] Buffer  The pointer to the buffer to free.

  @retval EFI_SUCCESS            The memory was returned to the system.
  @retval EFI_INVALID_PARAMETER  Buffer was invalid.

**/
STATIC
EFI_STATUS
EFIAPI
InternalFreePool (
  IN VOID  *Buffer
  )
{
  EFI_STATUS Status;

  Status = EFI_SUCCESS;

  if (!mDisableMemoryAllocationServices) {
    Status = mPrivateData.FreePool (Buffer);
  }

  return Status;
}

VOID
OverwriteFirmwareServices (
  VOID
  )
{
  EFI_STATUS Status;
  EFI_TPL    OldTpl;

  DEBUG_CODE (
    ASSERT (!mFirmwareServicesOverriden);
    );

  Status = EFI_SUCCESS;

  if (PcdGetBool (PcdMapVirtualPages)) {
    Status = VmAllocateMemoryPool ();
  }

  OldTpl = EfiRaiseTPL (TPL_HIGH_LEVEL);

  mPrivateData.GetMemoryMap = gBS->GetMemoryMap;
  gBS->GetMemoryMap         = InternalGetMemoryMap;

  if (PcdGetBool (PcdHandleGop)) {
    mPrivateData.HandleProtocol = gBS->HandleProtocol;
    gBS->HandleProtocol         = InternalHandleProtocol;
  }

  if (PcdGetBool (PcdDisableMemoryAllocationServicesBeforeExitBS)) {
    mPrivateData.AllocatePages = gBS->AllocatePages;
    gBS->AllocatePages         = InternalAllocatePages;

    mPrivateData.AllocatePool = gBS->AllocatePool;
    gBS->AllocatePool         = InternalAllocatePool;

    mPrivateData.ExitBootServices = gBS->ExitBootServices;
    gBS->ExitBootServices         = InternalExitBootServices;

    mPrivateData.FreePages = gBS->FreePages;
    gBS->FreePages         = InternalFreePages;

    mPrivateData.FreePool = gBS->FreePool;
    gBS->FreePool         = InternalFreePool;
  }

  UPDATE_EFI_TABLE_HEADER_CRC32 (gBS->Hdr);

  if (PcdGetBool (PcdPartialVirtualAddressMap)
   || (PcdGetBool (PcdMapVirtualPages) && !EFI_ERROR (Status))) {
    mPrivateData.SetVirtualAddressMap = gRT->SetVirtualAddressMap;
    gRT->SetVirtualAddressMap         = MoSetVirtualAddressMap;

    UPDATE_EFI_TABLE_HEADER_CRC32 (gRT->Hdr);
  } else {
    mPrivateData.SetVirtualAddressMap = NULL;
  }

  EfiRestoreTPL (OldTpl);

  DEBUG_CODE (
    mFirmwareServicesOverriden = TRUE;
    );
}

VOID
RestoreFirmwareServices (
  VOID
  )
{
  EFI_TPL OldTpl;

  DEBUG_CODE (
    ASSERT (mFirmwareServicesOverriden);
    );

  OldTpl = EfiRaiseTPL (TPL_HIGH_LEVEL);

  gBS->GetMemoryMap = mPrivateData.GetMemoryMap;

  if (PcdGetBool (PcdHandleGop)) {
    gBS->HandleProtocol = mPrivateData.HandleProtocol;
  }

  if (PcdGetBool (PcdDisableMemoryAllocationServicesBeforeExitBS)) {
    gBS->AllocatePages    = mPrivateData.AllocatePages;
    gBS->AllocatePool     = mPrivateData.AllocatePool;
    gBS->ExitBootServices = mPrivateData.ExitBootServices;
    gBS->FreePages        = mPrivateData.FreePages;
    gBS->FreePool         = mPrivateData.FreePool;
  }

  UPDATE_EFI_TABLE_HEADER_CRC32 (gBS->Hdr);

  if (mPrivateData.SetVirtualAddressMap != NULL) {
    gRT->SetVirtualAddressMap = mPrivateData.SetVirtualAddressMap;

    UPDATE_EFI_TABLE_HEADER_CRC32 (gRT->Hdr);
  }

  EfiRestoreTPL (OldTpl);

  DEBUG_CODE (
    mFirmwareServicesOverriden = FALSE;
    );
}
