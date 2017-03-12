;; @file
; Position-independent procedures for XNU kernel entry point hooking.
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

; The 16-byte stack alignment is defined by the UEFI IA-32 calling convention.
%define STACK_ALIGN 16

; Align a value (argument 1) to a boundary (argument 2).
%macro ALIGN_VALUE 2
    and     %1, ~(%2 - 1)
%endmacro

; Align the stack to prepare for arguments to be pushed.
%macro ALIGN_STACK_PRE_PUSH 1
    sub     esp, (STACK_ALIGN - (%1 * 4))
%endmacro

; Opcodes involving placeholder data

%define JMP_FAR_OP  db  0EAh
%define MOV_EAX_OP  db  0B8h 
%define PUSH_OP     db  068h

;
; Procedures
;

global ASM_PFX (KernelEntryHook32)
global ASM_PFX (KernelEntryHandler32)

; Procedure end labels

global ASM_PFX (gKernelEntryHook32End)
global ASM_PFX (gKernelEntryHandler32End)

; Placeholder addresses to be filled at runtime

global ASM_PFX (KernelEntryHook32).ASM_PFX (KernelEntryHandlerAddress)
global ASM_PFX (KernelEntryHandler32).ASM_PFX (KernelEntryAddress)
global ASM_PFX (KernelEntryHandler32).ASM_PFX (KernelHookHandoffAddress)
global ASM_PFX (KernelEntryHandler32).ASM_PFX (KernelEntryNotifyAddress)

    BITS    32
    CPU     386
    SECTION .text

;------------------------------------------------------------------------------
; VOID
; NORETURN
; KERNELENTRYAPI
; KernelEntryHook32 (
;   IN UINT32  BootArgs
;   );
;------------------------------------------------------------------------------
ASM_PFX (KernelEntryHook32):
    push    cs                  ; Save the caller cs.

    JMP_FAR_OP
.ASM_PFX (KernelEntryHandlerAddress):
    dd      0FFFFFFFFh
    dw      0000h               ; Flatten cs in preparation for the handler.
ASM_PFX (gKernelEntryHook32End):

;------------------------------------------------------------------------------
; VOID
; NORETURN
; KERNELENTRYAPI
; KernelEntryHandler32 (
;   IN UINT32  BootArgs
;   );
;------------------------------------------------------------------------------
ASM_PFX (KernelEntryHandler32):
    ;
    ; Assertion: cs is on the top of the stack.
    ; NOTE: This function does not save or change MMX registers.
    ;

    ; Push the kernel entry point address to stack so retf will jump there,
    ; this preserves the caller cs.  The entry address will be rebased later.
    PUSH_OP
.ASM_PFX (KernelEntryAddress):
    dd      0FFFFFFFFh

    ;
    ; Save context.
    ;

    ; Save the eflags register, the remaining segment selectors and all
    ; general-purpose registers.
    ;
    ; Note: When selectors are pushed, the stack is automatically aligned to
    ;       the next 32-bit boundary.
    pushfd                      ; 4 bytes
    push    ds                  ; 4 bytes
    push    es                  ; 4 bytes
    push    fs                  ; 4 bytes
    push    gs                  ; 4 bytes
    pushad                      ; 8 * 4 bytes
    mov     eax, cr0
    push    eax
    ; Total size: 52 bytes

    ; Rebase the return address to the caller cs.
    xor     ebx, ebx
    mov     bx, [ss:esp + 56]   ; Caller cs
    test    bx, bx
    jz      short .ASM_PFX (CallerCsIsFlat)
    shl     ebx, 4              ; Multiply with 16; Selectors move in 16-byte steps.
    sub     [ss:esp + 52], ebx  ; Subtract the selector start from the flat address.
.ASM_PFX (CallerCsIsFlat):
    ; Save the caller stack.
    mov     edi, esp

    sub     esp, 02h
    fstcw   word [esp]
    mov     word [esp + 2], 027Fh
    fldcw   word [esp + 2]

    ;
    ; Prepare compliance with the UEFI IA-32 calling convention.
    ;

    cld
    clts

    ALIGN_VALUE esp, STACK_ALIGN

    ; Flatten all segment selectors (cs is already flat, ss may not be as we
    ; are in protected mode).
    xor     dx, dx
    mov     ds, dx
    mov     es, dx
    mov     fs, dx
    mov     gs, dx

    ;
    ; Call the EFIAPI C code.
    ;

    ALIGN_STACK_PRE_PUSH 2

    ; Push the arguments to the stack.
    push    eax                 ; BootArgs
    PUSH_OP
.ASM_PFX (KernelHookHandoffAddress):
    dd      0FFFFFFFFh          ; Handoff

    ; Rebase esp onto ss = 0 as the UEFI IA-32 calling convention expects it to
    ; be flat.  This will not harm the alignment as selector registers are
    ; multiplied with 16.
    xor     ebx, ebx
    mov     bx, ss
    test    bx, bx
    jz      short .ASM_PFX (CallerSsIsFlat)
    shl     ebx, 4              ; Multiply with 16; Selectors move in 16-byte steps.
    add     esp, ebx            ; Add the selector start to the ss-based address.
.ASM_PFX (CallerSsIsFlat):

    MOV_EAX_OP
.ASM_PFX (KernelEntryNotifyAddress):
    dd      0FFFFFFFFh
    call    eax                 ; KernelEntryNotifyAddress

    ; Stack cleanup is not done as the caller stack will be restored.

    ;
    ; Restore the previously saved context.
    ;

    ; Restore the caller stack.
    mov     esp, edi

    ; Subtract 2 as esp has been restored to the value before the allocation.
    fldcw   word [esp - 2]

    ; Restore the original registers.  This ensures a seamless transition.
    pop     eax
    mov     cr0, eax
    popad
    pop     gs
    pop     fs
    pop     es
    pop     ds
    popfd

    ;
    ; Jump to the kernel.
    ;

    ; This will restore the caller cs.
    retf
ASM_PFX (gKernelEntryHandler32End):
