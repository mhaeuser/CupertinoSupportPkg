## @file
# Copyright (C) 2016 - 2017, CupertinoNet.  All rights reserved.<BR>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#
##

[Defines]
  PLATFORM_NAME           = CupertinoSupport
  PLATFORM_GUID           = 7AC724CD-7F2A-4F8D-A72A-BBCA4F6E1384
  PLATFORM_VERSION        = 1.0
  SUPPORTED_ARCHITECTURES = IA32|IPF|X64|EBC|ARM|AARCH64
  BUILD_TARGETS           = RELEASE|DEBUG|NOOPT
  SKUID_IDENTIFIER        = DEFAULT
  DSC_SPECIFICATION       = 0x00010006

  # TODO: Cleanup...

[LibraryClasses]
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  MiscMemoryLib|EfiMiscPkg/Library/MiscMemoryLib/MiscMemoryLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  EfiBootServicesLib|EfiMiscPkg/Library/EfiBootServicesLib/EfiBootServicesLib.inf
  MiscRuntimeLib|EfiMiscPkg/Library/MiscRuntimeLibNull/MiscRuntimeLibNull.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  AppleMachoLib|AppleXnuPkg/Library/AppleMachoLib/AppleMachoLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf

  MiscDevicePathLib|EfiMiscPkg/Library/MiscDevicePathLib/MiscDevicePathLib.inf
  MiscEventLib|EfiMiscPkg/Library/MiscEventLib/MiscEventLib.inf
 
  XnuSupportMemoryAllocationLib|CupertinoSupportPkg/Library/XnuSupportMemoryAllocationLib/XnuSupportMemoryAllocationLib.inf
  KernelEntryNotifyImageLib|CupertinoSupportPkg/Library/KernelEntryNotifyImageLib/KernelEntryNotifyImageLib.inf
  CupertinoXnuLib|CupertinoXnuPkg/Library/CupertinoXnuLib/CupertinoXnuLib.inf
  CupertinoFatBinaryLib|CupertinoXnuPkg/Library/CupertinoFatBinaryLib/CupertinoFatBinaryLib.inf
  FirmwareFixesLib|CupertinoSupportPkg/Library/FirmwareFixesLib/FirmwareFixesLib.inf

[LibraryClasses.IA32, LibraryClasses.X64]
  KernelEntryHookLib|CupertinoSupportPkg/Library/KernelEntryHookLib/KernelEntryHookLib.inf
  KernelEntryNotifyLib|CupertinoSupportPkg/Library/KernelEntryNotifyLib/KernelEntryNotifyLib.inf

[LibraryClasses.ARM, LibraryClasses.AARCH64, LibraryClasses.IPF, LibraryClasses.EBC]
  KernelEntryHookLib|CupertinoSupportPkg/Library/KernelEntryHookLibNull/KernelEntryHookLib.inf
  KernelEntryNotifyLib|CupertinoSupportPkg/Library/KernelEntryNotifyLibNull/KernelEntryNotifyLib.inf

[LibraryClasses.X64]
  VirtualMemoryLib|CupertinoSupportPkg/Library/VirtualMemoryLib/VirtualMemoryLib.inf

[LibraryClasses.IA32, LibraryClasses.ARM, LibraryClasses.AARCH64, LibraryClasses.IPF, LibraryClasses.EBC]
  VirtualMemoryLib|CupertinoSupportPkg/Library/VirtualMemoryLibNull/VirtualMemoryLib.inf

[LibraryClasses.ARM, LibraryClasses.AARCH64]
  NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf

  # Add support for GCC stack protector
  NULL|MdePkg/Library/BaseStackCheckLib/BaseStackCheckLib.inf

[Components]
  CupertinoSupportPkg/Driver/AppleBooterNotifyDxe/AppleBooterNotifyDxe.inf
  CupertinoSupportPkg/Driver/BlessDxe/BlessDxe.inf
  CupertinoSupportPkg/Driver/FatBinaryDxe/FatBinaryDxe.inf
  CupertinoSupportPkg/Library/FirmwareFixesLib/FirmwareFixesLib.inf
  CupertinoSupportPkg/Library/XnuSupportMemoryAllocationLib/XnuSupportMemoryAllocationLib.inf
  CupertinoSupportPkg/Library/KernelEntryNotifyImageLib/KernelEntryNotifyImageLib.inf

[Components.IA32, Components.X64]
  CupertinoSupportPkg/Library/KernelEntryHookLib/KernelEntryHookLib.inf
  CupertinoSupportPkg/Library/KernelEntryNotifyLib/KernelEntryNotifyLib.inf
  CupertinoSupportPkg/Library/VirtualMemoryLib/VirtualMemoryLib.inf
