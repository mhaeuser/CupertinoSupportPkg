;; @file
; Helper procedures to pass data to the XNU kernel hook procedures.
;
; Copyright (C) 2017, CupertinoNet.  All rights reserved.<BR>
; Portions Copyright (C) 2012 - 2014, Damir Mažar.  All rights reserved.<BR>
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.
;
;;

extern ASM_PFX (KernelEntryHook32).ASM_PFX (KernelEntryHandlerAddress)
extern ASM_PFX (KernelEntryHandler32).ASM_PFX (KernelEntryAddress)
extern ASM_PFX (KernelEntryHandler32).ASM_PFX (KernelHookHandoffAddress)
extern ASM_PFX (KernelEntryHandler32).ASM_PFX (KernelEntryNotifyAddress)

    DEFAULT REL
    BITS    64
    CPU     X64
    SECTION .text

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; PrepareKernelEntryHook32 (
;   IN UINT32  KernelEntry,
;   IN UINT32  HandlerEntry,
;   IN UINT32  NotifyEntry,
;   IN UINT32  Context
;   );
;------------------------------------------------------------------------------
global ASM_PFX (PrepareKernelEntryHook32)
ASM_PFX (PrepareKernelEntryHook32):
    ; Patch KernelEntryHandler32() with the address of the kernel entry point.
    mov     dword [ASM_PFX (KernelEntryHandler32).KernelEntryAddress], ecx

    ; Patch KernelEntryHook32() with the handler address.
    mov     dword [ASM_PFX (KernelEntryHook32).KernelEntryHandlerAddress], edx

    ; Patch KernelEntryHandler32() with the notify entry point.
    mov     dword [ASM_PFX (KernelEntryHandler32).ASM_PFX (KernelEntryNotifyAddress)], r8d

    ; Patch KernelEntryHandler32() with the context data address.
    mov     dword [ASM_PFX (KernelEntryHandler32).ASM_PFX (KernelHookHandoffAddress)], r9d

    retn
