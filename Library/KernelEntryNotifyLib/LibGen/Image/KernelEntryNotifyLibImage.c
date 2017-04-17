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

#include <Base.h>

#include <Library/AppleBootArgsLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/KernelEntryHookLib.h>

VOID
RestoreKernelEntry (
  IN VOID   *KernelEntry,
  IN VOID   *OriginalData,
  IN UINTN  Size
)
{
  ASSERT (KernelEntry != 0);
  ASSERT (OriginalData != NULL);
  ASSERT (Size > 0);

  CopyMem (KernelEntry, OriginalData, Size);
}

VOID
EFIAPI
_ModuleEntryPoint (
  IN KERNEL_ENTRY_HOOK_CONTEXT  *Context,
  IN BOOT_ARGS                  *BootArgs
  )
{
  ASSERT (BootArgs != NULL);
  ASSERT (Context != NULL);
  ASSERT (Context->Hdr.Signature == KERNEL_ENTRY_HOOK_CONTEXT_SIGNATURE);

  RestoreKernelEntry (
    (VOID *)Context->Hdr.KernelEntry,
    (VOID *)&Context->KernelEntryBackup[0],
    (UINTN)Context->Hdr.KernelEntrySize
    );

  // Relocate relocation blocks?

  // Run other C code...
}
