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

#ifndef KERNEL_ENTRY_HOOK_LIB_H_
#define KERNEL_ENTRY_HOOK_LIB_H_

#define KERNEL_ENTRY_HOOK_CONTEXT_SIGNATURE  SIGNATURE_32 ('H', 'N', 'D', 'O')

#pragma pack (4)

typedef struct {
  struct {
    UINT32 Signature;
    UINT32 KernelEntry;
    UINT32 KernelEntrySize;
  }     Hdr;
  UINT8 KernelEntryBackup[1];
} KERNEL_ENTRY_HOOK_CONTEXT;

#pragma pack ()

#endif // KERNEL_ENTRY_HOOK_LIB_H_
