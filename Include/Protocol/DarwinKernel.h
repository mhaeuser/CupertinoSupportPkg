/** @file
  Copyright (C) 2016 CupertinoNet.  All rights reserved.<BR>

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

#ifndef DARWIN_KERNEL_H_
#define DARWIN_KERNEL_H_

// Related definitions

// DARWIN_KERNEL_IMAGE_TYPE
enum {
  DarwinKernelImageTypeMacho       = 0,
  DarwinKernelImageTypeHibernation = 1
};

typedef UINTN DARWIN_KERNEL_IMAGE_TYPE;

// DARWIN_KERNEL_HEADER
typedef union {
  MACH_HEADER               *MachHeader;
  MACH_HEADER_64            *MachHeader64;
  IO_HIBERNATE_IMAGE_HEADER *HibernationImageHeader;
} DARWIN_KERNEL_HEADER;

// DARWIN_KERNEL_INFORMATION
typedef struct {
  DARWIN_KERNEL_IMAGE_TYPE ImageType;
  DARWIN_KERNEL_HEADER     Image;
} DARWIN_KERNEL_INFORMATION;

// DARWIN_KERNEL_DISCOVERED_CALLBACK
typedef
VOID
(EFIAPI *DARWIN_KERNEL_DISCOVERED_CALLBACK)(
  IN XNU_PHYSICAL_ADDRESS  KernelEntry
  );

// DARWIN_KERNEL_ENTRY_CALLBACK
typedef
VOID
(EFIAPI *DARWIN_KERNEL_ENTRY_CALLBACK)(
  VOID
  );

// Protocol definition

typedef DARWIN_KERNEL_PROTOCOL DARWIN_KERNEL_PROTOCOL;

// DARWIN_KERNEL_ADD_KERNEL_DISCOVERED_CALLBACK
typedef
EFI_STATUS
(EFIAPI *DARWIN_KERNEL_ADD_KERNEL_DISCOVERED_CALLBACK)(
  IN DARWIN_KERNEL_PROTOCOL             *This,
  IN DARWIN_KERNEL_DISCOVERED_CALLBACK  NotifyFunction
  );

// DARWIN_KERNEL_ADD_KERNEL_ENTRY_CALLBACK
typedef
EFI_STATUS
(EFIAPI *DARWIN_KERNEL_ADD_KERNEL_ENTRY_CALLBACK)(
  IN DARWIN_KERNEL_PROTOCOL        *This,
  IN DARWIN_KERNEL_ENTRY_CALLBACK  NotifyFunction
  );

// DARWIN_KERNEL_GET_IMAGE_INFORMATION
typedef
EFI_STATUS
(EFIAPI *DARWIN_KERNEL_GET_IMAGE_INFORMATION)(
  IN  DARWIN_KERNEL_PROTOCOL     *This,
  OUT DARWIN_KERNEL_INFORMATION  *KernelInformation
  );

// DARWIN_KERNEL_PROTOCOL
struct DARWIN_KERNEL_PROTOCOL {
  UINTN                                        Revision;
  DARWIN_KERNEL_ADD_KERNEL_DISCOVERED_CALLBACK AddKernelDiscoveredCallback;
  DARWIN_KERNEL_ADD_KERNEL_ENTRY_CALLBACK      AddKernelEntryCallback;
  DARWIN_KERNEL_GET_IMAGE_INFORMATION          GetImageInformation;
};

#endif // DARWIN_KERNEL_H_
