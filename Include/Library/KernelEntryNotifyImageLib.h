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

#ifndef KERNEL_ENTRY_NOTIFY_IMAGE_LIB_H_
#define KERNEL_ENTRY_NOTIFY_IMAGE_LIB_H_

typedef
VOID
(EFIAPI *KERNEL_ENTRY_NOTIFY)(
  IN VOID  *Context
  );

typedef struct {
  LIST_ENTRY         *Link;
  KERNEL_ENTRY_NOTIFY NotifyFunction;
} KERNEL_HOOK_FUNCTION;

UINTN
LoadKernelEntryNotifyImage (
  IN VOID  *FileBuffer
  );

extern LIST_ENTRY gNotifys;

#endif // KERNEL_ENTRY_NOTIFY_IMAGE_LIB_H_
