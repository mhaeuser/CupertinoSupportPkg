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

#ifndef APPLE_BOOTER_EXIT_NAMED_EVENT_H_
#define APPLE_BOOTER_EXIT_NAMED_EVENT_H_

// APPLE_BOOTER_EXIT_NAMED_EVENT_GUID
#define APPLE_BOOTER_EXIT_NAMED_EVENT_GUID  \
  { 0x75045DF7, 0x10B2, 0x4E70, { 0x9D, 0x9D, 0x84, 0x5A, 0x14, 0xCF, 0x37, 0xAE } }

// gAppleBooterExitNamedEventGuid
extern EFI_GUID gAppleBooterExitNamedEventGuid;

#endif // APPLE_BOOTER_EXIT_NAMED_EVENT_H_
