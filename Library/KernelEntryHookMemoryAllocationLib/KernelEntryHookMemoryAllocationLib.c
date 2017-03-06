/** @file
  Copyright (C) 2017, CupertinoNet.  All rights reserved.<BR>

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
**/

#include <Uefi.h>

#include <Library/CupertinoXnuLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MiscMemoryLib.h>

STATIC
VOID *
InternalAllocateKernelHookMemory (
  IN UINTN             Size,
  IN  EFI_MEMORY_TYPE  MemoryType
  )
{
  VOID *Memory;

  ASSERT (Size > 0);
  ASSERT ((MemoryType > EfiReservedMemoryType)
       && (MemoryType < EfiMaxMemoryType));

  Memory = AllocatePagesFromTop (
             MemoryType,
             EFI_SIZE_TO_PAGES (Size),
             XNU_MAX_PHYSICAL_ADDRESS
             );

  ASSERT (Memory != NULL);
  ASSERT (((UINTN)Memory + Size) <= XNU_MAX_PHYSICAL_ADDRESS);

  return Memory;
}

VOID *
AllocateKernelHookCode (
  IN UINTN  Size
  )
{
  return InternalAllocateKernelHookMemory (
           EfiBootServicesCode,
           EFI_SIZE_TO_PAGES (Size)
           );
}

VOID *
AllocateKernelHookData (
  IN UINTN  Size
  )
{
  return InternalAllocateKernelHookMemory (
           EfiBootServicesData,
           EFI_SIZE_TO_PAGES (Size)
           );
}

VOID
FreeKernelHookMemory (
  IN VOID   *Buffer,
  IN UINTN  Size
  )
{
  ASSERT (Buffer != NULL);
  ASSERT (Size > 0);

  FreePages (Buffer, EFI_SIZE_TO_PAGES (Size));
}
