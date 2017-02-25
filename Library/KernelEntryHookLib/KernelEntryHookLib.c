/** @file
  Copyright (C) 2017, CupertinoNet.  All rights reserved.<BR>
  Portions Copyright (C) 2012 - 2014, Damir Mažar.  All rights reserved.<BR>

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

#include <Library/AppleMachoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CupertinoXnuLib.h>
#include <Library/DebugLib.h>
#include <Library/MiscMemoryLib.h>
#include <Library/KernelEntryNotifyLib.h>
#include <Library/KernelEntryHookLib.h>
#include <Library/KernelEntryHookMemoryAllocationLib.h>
#include <Library/KernelEntryNotifyImageLib.h>

#include "KernelEntryHookInternal.h"

// KernelEntryHook
VOID
KernelEntryHook (
  IN OUT UINTN  KernelEntryAddress
  )
{
  // XNU Kernel transition always happens in protected mode.  This is an
  // assertion made by this function.

  UINTN                     NotifyEntry;
  UINTN                     MemorySize;
  VOID                      *Memory;
  KERNEL_ENTRY_HOOK_CONTEXT *Context;
  VOID                      *HandlerEntry;

  ASSERT (KernelEntryAddress != 0);
  ASSERT (KERNEL_ENTRY_HOOK_32_SIZE > 0);
  ASSERT (KERNEL_ENTRY_HANDLER_32_SIZE > 0);

  NotifyEntry = LoadKernelEntryNotifyImage (
                  gKernelEntryNotifyImageData
                  );

  if (NotifyEntry != 0) {
    MemorySize = (sizeof (*Context) + KERNEL_ENTRY_HOOK_32_SIZE);

    Memory = AllocateKernelHookData (MemorySize);

    if (Memory != NULL) {
      HandlerEntry = AllocateKernelHookCode (KERNEL_ENTRY_HANDLER_32_SIZE);

      if (HandlerEntry != NULL) {
        Context                      = (KERNEL_ENTRY_HOOK_CONTEXT *)Memory;
        Context->Hdr.Signature       = KERNEL_ENTRY_HOOK_CONTEXT_SIGNATURE;
        Context->Hdr.KernelEntry     = (UINT32)KernelEntryAddress;
        Context->Hdr.KernelEntrySize = (UINT32)KERNEL_ENTRY_HOOK_32_SIZE;

        PrepareKernelEntryHook32 (
          Context->Hdr.KernelEntry,
          (UINT32)(UINTN)HandlerEntry,
          (UINT32)NotifyEntry,
          (UINT32)(UINTN)Context
          );

        CopyMem (
          (VOID *)&Context->KernelEntryBackup[0],
          (VOID *)(UINTN)Context->Hdr.KernelEntry,
          (UINTN)Context->Hdr.KernelEntrySize
          );

        CopyMem (
          (VOID *)(UINTN)Context->Hdr.KernelEntry,
          (VOID *)(UINTN)KernelEntryHook32,
          (UINTN)Context->Hdr.KernelEntrySize
          );
  
        CopyMem (
          HandlerEntry,
          (VOID *)(UINTN)KernelEntryHandler32,
          KERNEL_ENTRY_HANDLER_32_SIZE
          );
      } else {
        FreeKernelHookMemory (
          Memory,
          MemorySize
          );
      }
    }
  }
}

// KernelHookPatchEntry
VOID
KernelHookPatchEntry (
  IN OUT VOID   *KernelHeader,
  IN     UINTN  KernelSlide
  )
{
  UINTN KernelEntry;

  ASSERT (KernelHeader != NULL);

  KernelEntry = XnuGetEntryAddress (KernelHeader);

  ASSERT (KernelEntry != 0);

  if (KernelEntry != 0) {
    KernelEntryHook (KernelEntry + KernelSlide);
  }
}
