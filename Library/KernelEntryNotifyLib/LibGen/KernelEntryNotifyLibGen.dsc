## @file
# Copyright (C) 2017, CupertinoNet.  All rights reserved.<BR>
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
  PLATFORM_NAME           = KernelEntryNotifyLibGen
  PLATFORM_GUID           = F94588B1-DBAB-44A6-9BAA-4B02A12DA02E
  PLATFORM_VERSION        = 1.0
  SUPPORTED_ARCHITECTURES = IA32|IPF|X64|EBC|ARM|AARCH64
  BUILD_TARGETS           = RELEASE|DEBUG|NOOPT
  SKUID_IDENTIFIER        = DEFAULT
  DSC_SPECIFICATION       = 0x00010006

[LibraryClasses]
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

[Components]
  CupertinoSupportPkg/Library/KernelEntryNotifyLib/LibGen/KernelEntryNotifyLib.inf
  CupertinoSupportPkg/Library/KernelEntryNotifyLib/LibGen/Data/KernelEntryNotifyLibData.inf

[Components.IA32]
  CupertinoSupportPkg/Library/KernelEntryNotifyLib/LibGen/Image/KernelEntryNotifyLibImage.inf
