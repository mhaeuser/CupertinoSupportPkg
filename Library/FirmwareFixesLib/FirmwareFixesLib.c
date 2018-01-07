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

#include <Protocol/AppleBooterHandle.h>

#include <Library/DebugLib.h>
#include <Library/EfiBootServicesLib.h>
#include <Library/MiscEventLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>

#include "FirmwareFixesInternal.h"

STATIC EFI_EVENT mAppleBooterHandleNotifyEvent   = NULL;
STATIC VOID      *mAppleBooterHandleRegistration = NULL;

STATIC UINTN mAppleBooterLevel = 0;

GLOBAL_REMOVE_IF_UNREFERENCED
BOOLEAN mXnuPrepareStartSignaledInCurrentBooter = FALSE;

/**
  Invoke a notification event

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  The pointer to the notification function's context,
                      which is implementation-dependent.

**/
STATIC
VOID
EFIAPI
InternalAppleBooterExitNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  --mAppleBooterLevel;

  if (mAppleBooterLevel == 0) {
    if (PcdGetBool (PcdPreserveSystemTable)) {
      InternalFreeSystemTableCopy ();
    }

    RestoreFirmwareServices ();

    EfiCloseEvent (Event);
  }
}

/**
  Invoke a notification event

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  The pointer to the notification function's context,
                      which is implementation-dependent.

**/
STATIC
VOID
EFIAPI
InternalAppleBooterStartNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  //
  // The allocation might have failed, so run thus on any level.
  //
  if (PcdGetBool (PcdPreserveSystemTable)) {
    InternalOverrideSystemTable (mAppleBooterHandleRegistration);
  }

  DEBUG_CODE (
    mXnuPrepareStartSignaledInCurrentBooter = FALSE;
    );

  if (mAppleBooterLevel == 0) {
    OverrideFirmwareServices ();
 
    EfiNamedEventListen (
      &gAppleBooterExitNamedEventGuid,
      TPL_NOTIFY,
      InternalAppleBooterExitNotify,
      NULL,
      NULL
      );
  }

  ++mAppleBooterLevel;
}

VOID
FirmwareFixesLibConstructor (
  VOID
  )
{
  EFI_EVENT Event;

  Event = MiscCreateNotifySignalEvent (
            InternalAppleBooterStartNotify,
            NULL
            );

  if (Event != NULL) {
    mAppleBooterHandleNotifyEvent = Event;

    EfiRegisterProtocolNotify (
      &gAppleBooterHandleProtocolGuid,
      mAppleBooterHandleNotifyEvent,
      &mAppleBooterHandleRegistration
      );
  }
}

VOID
FirmwareFixesLibDestructor (
  VOID
  )
{
  if (mAppleBooterHandleNotifyEvent != NULL) {
    EfiCloseEvent (mAppleBooterHandleNotifyEvent);

    mAppleBooterHandleNotifyEvent = NULL;
  }
}
