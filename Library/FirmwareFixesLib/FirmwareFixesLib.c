/** @file
  Copyright (C) 2012 - 2014 Damir Mažar.  All rights reserved.<BR>
  Portions Copyright (C) 2015 - 2017, CupertinoNet.  All rights reserved.<BR>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in
  the documentation and/or other materials provided with the
  distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
**/

#include <Uefi.h>

#include <Guid/AppleBooterExitNamedEvent.h>
#include <Guid/XnuPrepareStartNamedEvent.h>

#include <Protocol/AppleBooterHandle.h>
#include <Protocol/LoadedImage.h>

#include <Library/DebugLib.h>
#include <Library/EfiBootServicesLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include "FirmwareFixesInternal.h"

// FIRMWARE_FIXES_PRIVATE_DATA
typedef struct {
  EFI_EVENT                   AppleBooterHandleNotifyEvent;
  VOID                        *AppleBooterHandleRegistration;
} FIRMWARE_FIXES_PRIVATE_DATA;

// mPrivateData
STATIC FIRMWARE_FIXES_PRIVATE_DATA mPrivateData;

// AppleBooterExitNotify
/** Invoke a notification event

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  The pointer to the notification function's context,
                      which is implementation-dependent.
**/
VOID
EFIAPI
AppleBooterExitNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  RestoreFirmwareServices ();

  EfiCloseEvent (Event);
}

// AppleBooterStartNotify
/** Invoke a notification event

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  The pointer to the notification function's context,
                      which is implementation-dependent.
**/
VOID
EFIAPI
AppleBooterStartNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  FIRMWARE_FIXES_PRIVATE_DATA *PrivateData;
  EFI_STATUS                  Status;
  EFI_HANDLE                  BooterHandle;
  EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage;

  ASSERT (Context != NULL);

  PrivateData = (FIRMWARE_FIXES_PRIVATE_DATA *)Context;

  mXnuPrepareStartSignaledInCurrentBooter = FALSE;

  if (PcdGetBool (PcdPreserveOriginalSystemTable)) {
    Status = EfiLocateProtocol (
               &gAppleBooterHandleProtocolGuid,
               PrivateData->AppleBooterHandleRegistration,
               (VOID **)&BooterHandle
               );

    ASSERT (Status != EFI_NOT_FOUND);

    Status = EfiHandleProtocol (
               BooterHandle,
               &gEfiLoadedImageProtocolGuid,
               (VOID **)&LoadedImage
               );

    ASSERT (Status != EFI_UNSUPPORTED);

    // TODO: Implement.
    //LoadedImage->SystemTable = (EFI_SYSTEM_TABLE *)(UINTN)mSTCopy;
  }

  OverwriteFirmwareServices ();
 
  EfiNamedEventListen (
    &gAppleBooterExitNamedEventGuid,
    TPL_NOTIFY,
    AppleBooterExitNotify,
    NULL,
    NULL
    );
}

// FirmwareFixesLibConstructor
VOID
FirmwareFixesLibConstructor (
  VOID
  )
{
  EfiCreateEvent (
    EVT_NOTIFY_SIGNAL,
    TPL_NOTIFY,
    AppleBooterStartNotify,
    (VOID *)&mPrivateData,
    &mPrivateData.AppleBooterHandleNotifyEvent
    );

  EfiRegisterProtocolNotify (
    &gAppleBooterHandleProtocolGuid,
    mPrivateData.AppleBooterHandleNotifyEvent,
    &mPrivateData.AppleBooterHandleRegistration
    );
}

// FirmwareFixesLibDestructor
VOID
FirmwareFixesLibDestructor (
  VOID
  )
{
  EfiCloseEvent (mPrivateData.AppleBooterHandleNotifyEvent);
}