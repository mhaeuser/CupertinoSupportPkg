;; @file
; Some assembler helper functions plus boot.efi kernel jump callback
;
; Copyright (C) 2014 Sergey Slice.  All rights reserved.<BR>
; Copyright (C) 2014 Damir Mazar.  All rights reserved.<BR>
; Portions Copyright (C) 2015 - 2016 CupertinoNet.  All rights reserved.<BR>
,
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
;
; 1. Redistributions of source code must retain the above copyright
; notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright
; notice, this list of conditions and the following disclaimer in
; the documentation and/or other materials provided with the
; distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
; "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
; LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
; FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
; COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
; CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
; LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
; ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; POSSIBILITY OF SUCH DAMAGE.
;;

struc XDTR
    Limit	resw	1
    Base	resq	1
endstruc

; C callback method called on jump to kernel after boot.efi finishes 
extern	ASM_PFX (KernelEntryPatchJumpBack)

; saved 64bit state
global ASM_PFX (gAsmSavedGdtr)
global ASM_PFX (gAsmSavedIdtr)
global ASM_PFX (gAsmSavedCr3)

; addresses of relocated gAsmCopyAndJumpToKernel code - filled by AsmPrepareAsmJumpFromKernel()
global ASM_PFX (gAsmCopyAndJumpToKernel32Address)
global ASM_PFX (gAsmCopyAndJumpToKernel64Address)

; kernel entry address - filled by KernelEntryPatchJump()
global ASM_PFX (gAsmKernelEntry)

; params for kernel image relocation - filled by KernelEntryPatchJumpBack()
global ASM_PFX (gAsmKernelImageStartReloc)
global ASM_PFX (gAsmKernelImageStart)
global ASM_PFX (gAsmKernelImageSize)

; end of gAsmEntryPatchCode func
global ASM_PFX (gAsmEntryPatchCodeEnd)

; start and end of gAsmCopyAndJumpToKernel
global ASM_PFX (gAsmCopyAndJumpToKernel)
global ASM_PFX (gAsmCopyAndJumpToKernel32)
global ASM_PFX (gAsmCopyAndJumpToKernel64)
global ASM_PFX (gAsmCopyAndJumpToKernelEnd)

global ASM_PFX (gAsmEntryPatchCodeAsmJumpFromKernelPlaceholder)


;SECTION	.data
SECTION .text

;
; Adding data to code segment to avoid some error:
; Undefined symbols for architecture x86_64:
;  "_gAsmSavedGdtr", referenced from:
;      _AsmPrepareAsmJumpFromKernel in OsxAptioFixDrv.lib(AsmFuncsX64.obj)
;

; variables accessed from both 32 and 64 bit code
; need to have this exactly in this order
DataBase:

; 64 bit state
gAsmSavedGdtrOff	EQU $-DataBase
ASM_PFX (gAsmSavedGdtr):
    dw	0
    dq	0

gAsmSavedIdtrOff	EQU $-DataBase
ASM_PFX (gAsmSavedIdtr):
    dw	0
    dq	0

        align 08h, db 0
gAsmSavedCr3Off		EQU	$-DataBase
ASM_PFX (gAsmSavedCr3):
    dq	0

SavedCSOff		EQU $-DataBase
SavedCS:
    dw	0

SavedDSOff		EQU $-DataBase
SavedDS:
    dw	0

; 32 bit state
gAsmSavedGdtr32Off	EQU $-DataBase
gAsmSavedGdtr32:
    dw 0
    dq 0


gAsmSavedIdtr32Off	EQU	$-DataBase
gAsmSavedIdtr32:
    dw 0
    dq 0


SavedCS32Off	EQU $-DataBase
SavedCS32:		dw			0

SavedDS32Off	EQU $-DataBase
SavedDS32:		dw			0

SavedESP32Off	EQU $-DataBase
SavedESP32:		DD			0

        align 08h, db 0

; address of relocated gAsmCopyAndJumpToKernel32 - 64 bit
gAsmCopyAndJumpToKernel32AddressOff	EQU $-DataBase
ASM_PFX (gAsmCopyAndJumpToKernel32Address):
    dq	0

; address of relocated gAsmCopyAndJumpToKernel64 - 64 bit
gAsmCopyAndJumpToKernel64AddressOff	EQU $-DataBase
ASM_PFX (gAsmCopyAndJumpToKernel64Address):
    dq	0

; kernel entry - 64 bit
gAsmKernelEntryOff				EQU $-DataBase
ASM_PFX (gAsmKernelEntry):
    dq	0

;
; for copying kernel image from reloc block to proper mem place
;

; kernel image start in reloc block (source) - 64 bit
gAsmKernelImageStartRelocOff		EQU $-DataBase
ASM_PFX (gAsmKernelImageStartReloc):
    dq	0

; kernel image start (destination) - 64 bit
gAsmKernelImageStartOff			EQU $-DataBase
ASM_PFX (gAsmKernelImageStart):
    dq	0

; kernel image size - 64 bit
gAsmKernelImageSizeOff			EQU $-DataBase
ASM_PFX (gAsmKernelImageSize):
    dq	0

ASM_PFX (gAsmEntryPatchCodeAsmJumpFromKernelPlaceholder) DD 0

        align 08h, db 0

; GDT not used since we are reusing UEFI state
; but left here in case will be needed.
;
; GDR record
GDTROff			EQU $-DataBase

GDTR
    dw GDT_END - GDT_BASE - 1   ; GDT limit

GDTR_BASE
    dq 0                        ; GDT base - needs to be set in code

        align 08h, db 0
; GDT
GDT_BASE:
; null descriptor
NULL_SEL		equ $-GDT_BASE			; 0x00
    dw 0			; limit 15:0
    dw 0			; base 15:0
    db 0			; base 23:16
    db 0			; type
    db 0			; limit 19:16, flags
    db 0			; base 31:24

; 64 bit code segment descriptor
CODE64_SEL		equ $-GDT_BASE			; 0x08
        dw 0FFFFh		; limit 0xFFFFF
        dw 0			; base 0
        db 0
        db 09Ah			; P=1 | DPL=00 | S=1 (User) # Type=A=1010: Code/Data=1 | C:Conforming=0 | R:Readable=1 | A:Accessed=0
        db 0AFh			; Flags=A=1010: G:Granularity=1 (4K) | D:Default Operand Size=0 (in long mode) | L:Long=1 (64 bit) | AVL=0
        db 0

; 32 bit and 64 bit data segment descriptor (in 64 bit almost all is ignored, so can be reused)
DATA_SEL		equ $-GDT_BASE			; 0x10
        dw 0FFFFh		; limit 0xFFFFF
        dw 0			; base 0
        db 0
        db 092h			; P=1 | DPL=00 | S=1 (User) # Type=2=0010: Code/Data=0 | E:Expand-Down=0 | W:Writable=1 | A:Accessed=0
        db 0CFh			; Flags=C=1100: G:Granularity=1 (4K) | D/B=1 D not used when E=0, for stack B=1 means 32 bit stack | L:Long=0 not used | AVL=0
        db 0

; 32 bit code segment descriptor
CODE32_SEL		equ $-GDT_BASE			; 0x18
        dw 0FFFFh		; limit 0xFFFFF
        dw 0			; base 0
        db 0
        db 09Ah			; P=1 | DPL=00 | S=1 (User) # Type=A=1010: Code/Data=1 | C:Conforming=0 | R:Readable=1 | A:Accessed=0
        db 0CFh			; Flags=C=1100: G:Granularity=1 (4K) | D:Default Operand Size=0 (in long mode) | L:Long=0 (32 bit) | AVL=0
        db 0

GDT_END:


;SECTION .text

BITS    64


;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; ReadSp (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX (ReadSp)
ASM_PFX (ReadSp):
    mov		rax, rsp
    add		rax, 8			; return SP as caller see it
    ret


;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmPrepareAsmJumpFromKernel (
;   );
;------------------------------------------------------------------------------
global ASM_PFX (AsmPrepareAsmJumpFromKernel)
ASM_PFX (AsmPrepareAsmJumpFromKernel):
    ; save 64 bit state
    sgdt	[REL ASM_PFX (gAsmSavedGdtr)]
    sidt	[REL ASM_PFX (gAsmSavedIdtr)]
    mov     rax, cr3
    mov     [REL ASM_PFX (gAsmSavedCr3)], rax
    mov		word [REL SavedCS], cs
    mov		word [REL SavedDS], ds

    ; pass DataBase to 32 bit code
    lea		rax, [REL DataBase]
    mov		dword [REL DataBaseAdr], eax;
    
    ; prepare gAsmEntryPatchCode:
    ; patch gAsmEntryPatchCode with address of AsmJumpFromKernel
    lea		rax, [REL ASM_PFX (AsmJumpFromKernel)]
    mov	dword [REL gAsmEntryPatchCodeAsmJumpFromKernelPlaceholder], eax

    ret


;------------------------------------------------------------------------------
; Code that is used for patching kernel entry to jump back
; to our code (to AsmJumpFromKernel):
; - load ecx (rcx) with address to AsmJumpFromKernel
; - jump to AsmJumpFromKernel
; The same generated opcode must run properly in both 32 and 64 bit.
; 64 bit:
; - we must set rcx to 0 (upper 4 bytes) before loading ecx with address (lower 4 bytes of rcx)
; - this requires xor %rcx, %rcx
; - and that opcode contains 0x48 in front of 32 bit xor %ecx, %ecx
; 32 bit:
; - 0x48 opcode is dec %eax in 32 bit
; - and then we must inc %eax later if 32 bit is detected in AsmJumpFromKernel
;
; This code is patched with address of AsmJumpFromKernel
; (into gAsmEntryPatchCodeAsmJumpFromKernelPlaceholder)
; and then copied to kernel entry address by KernelEntryPatchJump()
;------------------------------------------------------------------------------
global ASM_PFX (gAsmEntryPatchCode)
ASM_PFX (gAsmEntryPatchCode):
BITS    64
    xor		rcx, rcx							; -> 32-bit: dec  eax \ xor ecx, ecx
    mov     dword ecx, gAsmEntryPatchCodeAsmJumpFromKernelPlaceholder
    call	rcx
ASM_PFX (gAsmEntryPatchCodeEnd):


;------------------------------------------------------------------------------
; AsmJumpFromKernel
;
; Callback from boot.efi - this is where we jump when boot.efi jumps to kernel.
;
; - test if we are in 32 bit or in 64 bit
; - if 64 bit, then jump to AsmJumpFromKernel64
; - else just continue with AsmJumpFromKernel32
;------------------------------------------------------------------------------
global ASM_PFX (AsmJumpFromKernel)
ASM_PFX (AsmJumpFromKernel):

; writing in 32 bit, but code must run in 64 bit also
BITS    32
    push	eax                     ; save bootArgs pointer to stack
    mov 	dword ecx, 0c0000080h	; EFER MSR number.
    rdmsr							; Read EFER.
    bt		eax, 8                  ; Check if LME==1 -> CF=1.
    pop		eax
    jc		AsmJumpFromKernel64	; LME==1 -> jump to 64 bit code due to longmode
    ; otherwise, continue with AsmJumpFromKernel32
    ; but first add 1 to it since it was decremented in 32 bit
    ; in gAsmEntryPatchCode
    inc		eax

    ; test the above code in 64 bit - above 32 bit code gives opcode
    ; that is equivalent to following in 64 bit
;BITS    64
;	push	rax                     ; save bootArgs pointer to stack
;	movl 	ecx, 0c0000080h         ; EFER MSR number.
;	rdmsr							; Read EFER.
;	bt		eax, 8                  ; Check if LME==1 -> CF=1.
;	pop		rax
;	jnc		AsmJumpFromKernel64	; LME==1 -> jump to 64 bit code


;------------------------------------------------------------------------------
; AsmJumpFromKernel32
; 
; Callback from boot.efi in 32 bit mode.
; State is prepared for kernel: 32 bit, no paging, pointer to bootArgs in eax.
;
; MS 64 bit compiler generates only 64 bit opcode, but this function needs
; combined 32 and 64 bit code. Code can be written only with 64 bit instructions,
; but generated opcode must be valid 32 bit. This is a big issue.
; Well, I guess I know now how the guys in Intel are feeling when
; they have to work with MS tools on a similar code.
;
; Another problem is that it's not possible to access saved variables
; from 32 bit code (64 bit code has relative addresses, but 32 bit does not
; and depends on fixes during load and that is not happening sice
; generated code is marked 64 bit, or something similar).
; To overcome this, starting address of ou DataBase is passed in runtime - stored
; to DataBaseAdr below as an argument to mov. 
;------------------------------------------------------------------------------
AsmJumpFromKernel32:
    
BITS    32
    ;hlt	; uncomment to stop here for test
    ; save bootArgs pointer to edi
    mov		edi, eax
    
    ; load ebx with DataBase - we'll access our saved data with it
    db		0BBh				; mov ebx, OFFSET DataBase
DataBaseAdr	dd	0

    ; let's find out kernel entry point - we'll need it to jump back.
    ; we are called with
    ;   dec		eax
    ;   xor		ecx, ecx
    ;   mov 	ecx, 011223344h
    ;   call ecx
    ; and that left return addr on stack. those instructions
    ; are 10 bytes long, and if we take address from stack and
    ; substitute 10 from it, we will get kernel entry point.
    pop		ecx							; 32 bit: pop ecx
    sub		ecx, 10
    ; and save it
    mov		dword [ebx + gAsmKernelEntryOff], ecx

    ; lets save 32 bit state to be able to recover it later
    sgdt	[ebx + gAsmSavedGdtr32Off]
    sidt	[ebx + gAsmSavedIdtr32Off]
    mov		word [ebx + SavedCS32Off], cs
    mov		word [ebx + SavedDS32Off], ds
    mov		dword [ebx + SavedESP32Off], esp

    ;
    ; move to 64 bit mode ...
    ;
    
    ; load saved UEFI GDT, IDT
    ; will become active after code segment is changed in long jump
    ; rbx is ebx in 32 bit
    lgdt	[ebx + gAsmSavedGdtrOff]
    lidt	[ebx + gAsmSavedIdtrOff]

    ; enable the 64-bit page-translation-table entries by setting CR4.PAE=1
    mov		eax, cr4
    bts		eax, 5
    mov		cr4, eax
    
    ; set the long-mode page tables - reuse saved UEFI tables
    mov		eax, dword [ebx +gAsmSavedCr3Off]
    mov		cr3, eax
    
    ; enable long mode (set EFER.LME=1).
    mov 	ecx, 0c0000080h			; EFER MSR number.
    rdmsr							; Read EFER.
    bts		eax, 8					; Set LME=1.
    wrmsr							; Write EFER.

    ; enable paging to activate long mode (set CR0.PG=1)
    mov		eax, cr0				; Read CR0.
    bts		eax, 31					; Set PG=1.
    mov		cr0, eax				; Write CR0.
    
    ; jump to the 64-bit code segment
    mov		ax, word [ebx + SavedCSOff]
    push 	eax
    call	_RETF32
    
    ;
    ; aloha!
    ; if there is any luck, we are in 64 bit mode now
    ;
    ;hlt	; uncomment to stop here for test
BITS 64

    ; set segmens
    mov		ax, word [rbx + SavedDSOff]
    mov		ds, ax
    ; set up stack ...
    ; not sure if needed, but lets set ss to ds
    mov		ss, ax
    ; lets align the stack
    mov		rax, rsp
    and		rax, 0fffffffffffffff8h
    mov		rsp, rax
    
    ; call our C code
    ; (calling conv.: always reserve place for 4 args on stack)
    ; KernelEntryPatchJumpBack (rcx = rax = bootArgs, rdx = 0 = 32 bit kernel jump)
    mov		rcx, rdi
    xor		rdx, rdx
    push	rdx
    push	rdx
    push	rdx
    push	rcx

; TEST 64 bit jump
;	mov		rax, rdi
;	mov		rdx, qword [gAsmKernelEntry]
;	jmp		rdx
; TEST end

    ; KernelEntryPatchJumpBack should be EFIAPI
    ; and rbx should not be changed by EFIAPI calling convention
    call	ASM_PFX (KernelEntryPatchJumpBack)
    ;hlt	; uncomment to stop here for test
    ; return value in rax is bootArgs pointer
    mov		rdi, rax
    
    ;
    ; time to go back to 32 bit
    ;
    
    ; load saved 32 bit gdtr
    lgdt	[rbx + gAsmSavedGdtr32Off]
    ; push saved cs and rip (with call) to stack and do retf
    mov		ax, WORD [rbx + SavedCS32Off]
    push 	rax
    call	_RETF64

    ;
    ; ok, 32 bit opcode again from here
    ;
BITS    32

    ; disable paging (set CR0.PG=0)
    mov		eax, cr0				; Read CR0.
    btr		eax, 31					; Set PG=0.
    mov		cr0, eax				; Write CR0.
    
    ; disable long mode (set EFER.LME=0).
    mov 	ecx, 0c0000080h			; EFER MSR number.
    rdmsr							; Read EFER.
    btr		eax, 8					; Set LME=0.
    wrmsr							; Write EFER.
    jmp		toNext
toNext:	
    ;
    ; we are in 32 bit protected mode, no paging
    ;

    ; now reload saved 32 bit state data
    lidt	[ebx + gAsmSavedIdtr32Off]
    mov		ax, word [ebx + SavedDS32Off]
    mov		ss, ax
    mov		ds, ax
    mov		es, ax
    mov		fs, ax
    mov		gs, ax
    mov		esp, dword [ebx + SavedESP32Off]
    
    ;
    ; prepare vars for copying kernel to proper mem
    ; and jump to kernel: set registers as needed
    ; by gAsmCopyAndJumpToKernel32
    ;
    
    ; boot args back from edi
    mov		eax, edi
    ; kernel entry point
    mov		edx, dword [ebx + gAsmKernelEntryOff]
    
    ; source, destination and size for kernel copy
    mov		esi, dword [ebx + gAsmKernelImageStartRelocOff]
    mov		edi, dword [ebx + gAsmKernelImageStartOff]
    mov		ecx, dword [ebx + gAsmKernelImageSizeOff]
    
    ; address of relocated gAsmCopyAndJumpToKernel32
    mov		ebx, dword [ebx + gAsmCopyAndJumpToKernel32AddressOff]
    ; note: ebx not valid as a pointer to DataBase any more
    
    ;
    ; jump to gAsmCopyAndJumpToKernel32
    ;
    jmp		ebx
    

_RETF64:
    DB	048h
_RETF32:
    retf


;------------------------------------------------------------------------------
; AsmJumpFromKernel64
;
; Callback from boot.efi in 64 bit mode.
; State is prepared for kernel: 64 bit, pointer to bootArgs in rax.
;------------------------------------------------------------------------------
AsmJumpFromKernel64:

BITS    64
    ; let's find out kernel entry point - we'll need it to jump back.
    pop		rcx
    sub		rcx, 10
    ; and save it
    mov		qword [REL ASM_PFX (gAsmKernelEntry)], rcx

    ; call our C code
    ; (calling conv.: always reserve place for 4 args on stack)
    ; KernelEntryPatchJumpBack (rcx = rax = bootArgs, rdx = 1 = 64 bit kernel jump)
    mov		rcx, rax
    xor		rdx, rdx
    inc		edx
    push	rdx
    push	rdx
    push	rdx
    push	rcx
    ; KernelEntryPatchJumpBack should be EFIAPI
    call	ASM_PFX (KernelEntryPatchJumpBack)
    ;hlt	; uncomment to stop here for test
    ; return value in rax is bootArgs pointer

    ;
    ; prepare vars for copying kernel to proper mem
    ; and jump to kernel: set registers as needed
    ; by gAsmCopyAndJumpToKernel64
    ;

    ; kernel entry point
    mov		rdx, [REL ASM_PFX (gAsmKernelEntry)]

    ; source, destination and size for kernel copy
    mov		rsi, [REL ASM_PFX (gAsmKernelImageStartReloc)]
    mov		rdi, [REL ASM_PFX (gAsmKernelImageStart)]
    mov		rcx, [REL ASM_PFX (gAsmKernelImageSize)]

    ; address of relocated gAsmCopyAndJumpToKernel64
    mov		rbx, [REL ASM_PFX (gAsmCopyAndJumpToKernel64Address)]

    ;
    ; jump to gAsmCopyAndJumpToKernel64
    ;
    jmp		rbx
    ret
;AsmJumpFromKernel64   ENDP


;------------------------------------------------------------------------------
; gAsmCopyAndJumpToKernel 
; 
; This is the last part of the code - it will copy kernel image from reloc
; block to proper mem place and jump to kernel.
; There are separate versions for 32 and 64 bit.
; This code will be relocated (copied) to higher mem by AsmPrepareAsmJumpFromKernel().
;------------------------------------------------------------------------------
        align 08h
global ASM_PFX (gAsmCopyAndJumpToKernel)
ASM_PFX (gAsmCopyAndJumpToKernel):

;------------------------------------------------------------------------------
; gAsmCopyAndJumpToKernel32
;
; Expects:
; EAX = address of boot args (proper address, not from reloc block)
; EDX = kernel entry point
; ESI = start of kernel image in reloc block (source)
; EDI = proper start of kernel image (destination)
; ECX = kernel image size in bytes
;------------------------------------------------------------------------------
global ASM_PFX (gAsmCopyAndJumpToKernel32)
ASM_PFX (gAsmCopyAndJumpToKernel32):

BITS    32
    ;
    ; we will move double words (4 bytes)
    ; so ajust ECX to number of double words.
    ; just in case ECX is not multiple of 4 - inc by 1
    ;
    shr		ecx, 2
    inc     ecx

    ;
    ; copy kernel image from reloc block to proper mem place.
    ; all params should be already set:
    ; ECX = number of double words
    ; DS:ESI = source
    ; ES:EDI = destination
    ;
    cld								; direction is up
    rep movsd
    
    ;
    ; and finally jump to kernel:
    ; EAX already contains bootArgs pointer,
    ; and EDX contains kernel entry point
    ;
    ;hlt
    jmp		edx
;gAsmCopyAndJumpToKernel32   ENDP
gAsmCopyAndJumpToKernel32End:


;------------------------------------------------------------------------------
; gAsmCopyAndJumpToKernel64
;
; Expects:
; RAX = address of boot args (proper address, not from reloc block)
; RDX = kernel entry point
; RSI = start of kernel image in reloc block (source)
; RDI = proper start of kernel image (destination)
; RCX = kernel image size in bytes
;------------------------------------------------------------------------------
        align 08h
global ASM_PFX (gAsmCopyAndJumpToKernel64)
ASM_PFX (gAsmCopyAndJumpToKernel64):

BITS    64
    ;
    ; we will move quad words (8 bytes)
    ; so ajust RCX to number of double words.
    ; just in case RCX is not multiple of 8 - inc by 1
    ;
    shr		rcx, 3
    inc		rcx

    ;
    ; copy kernel image from reloc block to proper mem place.
    ; all params should be already set:
    ; RCX = number of double words
    ; RSI = source
    ; RDI = destination
    ;
    cld								; direction is up
    rep movsq

    ;
    ; and finally jump to kernel:
    ; RAX already contains bootArgs pointer,
    ; and RDX contains kernel entry point
    ;
    ; hlt
    jmp		rdx

;gAsmCopyAndJumpToKernel64	ENDP
gAsmCopyAndJumpToKernel64End:

global ASM_PFX (gAsmCopyAndJumpToKernelEnd)
ASM_PFX (gAsmCopyAndJumpToKernelEnd):
;END
    