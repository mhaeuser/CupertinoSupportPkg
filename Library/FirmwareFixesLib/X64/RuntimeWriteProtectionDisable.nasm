;; @file
; Runtime Services shim implementations that disable the CR0 Write Protection
; before handing off.
;
; Copyright (C) 2017, vit9696.  All rights reserved.<BR>
; Portions Copyright (C) 2017, CupertinoNet.  All rights reserved.<BR>
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

BITS     64
DEFAULT  REL

#define ASM_PFX(x) _x

SECTION .text

global ASM_PFX (gRtWpDisableShimsDataStart)
ASM_PFX (gRtWpDisableShimsDataStart):


global ASM_PFX (RtWpDisableSetVariable)
ASM_PFX (RtWpDisableSetVariable):
    push       rsi
    push       rbx
    sub        rsp, 0x38
    pushfq
    pop        rsi
    cli
    mov        rbx, cr0
    mov        rax, rbx
    and        rax, ~0x10000
    mov        cr0, rax
    mov        rax, qword [rsp+0x70]
    mov        qword [rsp+0x20], rax
    mov        rax, qword [ASM_PFX (gSetVariable)]
    call       rax
    test       ebx, 0x10000
    je         .SET_SKIP_RESTORE_WP
    mov        cr0, rbx
.SET_SKIP_RESTORE_WP:
    test       esi, 0x200
    je         .SET_SKIP_RESTORE_INTR
    sti
.SET_SKIP_RESTORE_INTR:
    add        rsp, 0x38
    pop        rbx
    pop        rsi
    ret

global ASM_PFX (RtWpDisableGetVariable)
ASM_PFX (RtWpDisableGetVariable):
    push       rsi
    push       rbx
    sub        rsp, 0x38
    pushfq
    pop        rsi
    cli
    mov        rbx, cr0
    mov        rax, rbx
    and        rax, ~0x10000
    mov        cr0, rax
    mov        rax, qword [rsp+0x70]
    mov        qword [rsp+0x20], rax
    mov        rax, qword [ASM_PFX (gGetVariable)]
    call       rax
    test       ebx, 0x10000
    je         .SET_SKIP_RESTORE_WP
    mov        cr0, rbx
.SET_SKIP_RESTORE_WP:
    test       esi, 0x200
    je         .SET_SKIP_RESTORE_INTR
    sti
.SET_SKIP_RESTORE_INTR:
    add        rsp, 0x38
    pop        rbx
    pop        rsi
    ret

global ASM_PFX (RtWpDisableGetNextVariableName)
ASM_PFX (RtWpDisableGetNextVariableName):
    push       rsi
    push       rbx
    sub        rsp, 0x28
    pushfq
    pop        rsi
    cli
    mov        rbx, cr0
    mov        rax, rbx
    and        rax, ~0x10000
    mov        cr0, rax
    mov        rax, qword [ASM_PFX (gGetNextVariableName)]
    call       rax
    test       ebx, 0x10000
    je         .NEXT_SKIP_RESTORE_WP
    mov        cr0, rbx
.NEXT_SKIP_RESTORE_WP:
    test       esi, 0x200
    je         .NEXT_SKIP_RESTORE_INTR
    sti
.NEXT_SKIP_RESTORE_INTR:
    add        rsp, 0x28
    pop        rbx
    pop        rsi
    ret


global ASM_PFX (gGetNextVariableName)
ASM_PFX (gGetNextVariableName):  dq  0

global ASM_PFX (gGetVariable)
ASM_PFX (gGetVariable):          dq  0

global ASM_PFX (gSetVariable)
ASM_PFX (gSetVariable):          dq  0


global ASM_PFX (gRtWpDisableShimsDataEnd)
ASM_PFX (gRtWpDisableShimsDataEnd):
