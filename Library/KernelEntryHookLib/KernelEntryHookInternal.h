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

#ifndef KERNEL_ENTRY_HOOK_INTERNAL_H_
#define KERNEL_ENTRY_HOOK_INTERNAL_H_

#define KERNELENTRYAPI

#define PROCEDURE_SIZE(Name) ((UINTN)&(g##Name##End) - (UINTN)&(Name))

VOID
EFIAPI
PrepareKernelEntryHook32 (
  IN UINT32  KernelEntry,
  IN UINT32  HandlerEntry,
  IN UINT32  NotifyEntry,
  IN UINT32  Context
  );

#define KERNEL_ENTRY_HOOK_32_SIZE  PROCEDURE_SIZE (KernelEntryHook32)

VOID
NORETURN
KERNELENTRYAPI
KernelEntryHook32 (
  IN UINT32  BootArgs
  );

extern UINTN gKernelEntryHook32End;

#define KERNEL_ENTRY_HANDLER_32_SIZE  PROCEDURE_SIZE (KernelEntryHandler32)

VOID
NORETURN
KERNELENTRYAPI
KernelEntryHandler32 (
  IN UINT32  BootArgs
  );

extern UINTN gKernelEntryHandler32End;

#endif // KERNEL_ENTRY_HOOK_INTERNAL_H_
