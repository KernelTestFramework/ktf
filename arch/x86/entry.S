/*
 * Copyright © 2020 Amazon.com, Inc. or its affiliates.
 * Copyright © 2014,2015 Citrix Systems Ltd.
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <asm-offsets.h>
#include <asm-macros.h>
#include <processor.h>
#include <segment.h>
#include <page.h>
#include <traps.h>
#include <usermode.h>
#include <errno.h>

.macro _handle_usermode cr3
    testb $0x3, cpu_exc_cs(%_ASM_SP)
    jz 1f /* skip if from kernel mode */
        SET_CR3 \cr3
        swapgs
    1:
.endm

.macro enter_from_usermode
    _handle_usermode cr3
.endm

.macro enter_to_usermode
    _handle_usermode user_cr3
.endm

.macro syscall_from_usermode
    SET_CR3 cr3
    swapgs
    SWITCH_STACK
.endm

.macro syscall_to_usermode
    SWITCH_STACK
    swapgs
    SET_CR3 user_cr3
.endm

.macro exception_handler sym vec has_error_code
ENTRY(entry_\sym)
    enter_from_usermode

    .if \has_error_code == 0
        push $0
    .endif

#if defined(__x86_64__)
    movl $\vec, cpu_exc_vector(%_ASM_SP)
#else
    push $\vec
#endif

    jmp handle_exception
END_FUNC(entry_\sym)
.endm

.macro MASK_USER_FLAGS
    PUSHF
    andl $~(USERMODE_FLAGS_MASK), (%_ASM_SP)
    POPF
.endm

.macro interrupt_handler sym func
ENTRY(asm_interrupt_handler_\sym)
    enter_from_usermode

    MASK_USER_FLAGS

    cld
    SAVE_ALL_REGS
    call \func
    RESTORE_ALL_REGS

    enter_to_usermode
    IRET
END_FUNC(asm_interrupt_handler_\sym)
.endm

ENTRY(handle_exception)
    cld
    SAVE_ALL_REGS

    mov %_ASM_SP, %_ASM_DI
    call do_exception

    RESTORE_ALL_REGS

    add $8, %_ASM_SP

    enter_to_usermode
    IRET
END_FUNC(handle_exception)

.align PAGE_SIZE
GLOBAL(exception_handlers)
exception_handler DE  X86_EX_DE  0
exception_handler DB  X86_EX_DB  0
exception_handler NMI X86_EX_NMI 0
exception_handler BP  X86_EX_BP  0
exception_handler OF  X86_EX_OF  0
exception_handler BR  X86_EX_BR  0
exception_handler UD  X86_EX_UD  0
exception_handler NM  X86_EX_NM  0
exception_handler DF  X86_EX_DF  1
exception_handler CS  X86_EX_CS  0
exception_handler TS  X86_EX_TS  1
exception_handler NP  X86_EX_NP  1
exception_handler SS  X86_EX_SS  1
exception_handler GP  X86_EX_GP  1
exception_handler PF  X86_EX_PF  1
exception_handler SPV X86_EX_SPV 0
exception_handler MF  X86_EX_MF  0
exception_handler AC  X86_EX_AC  1
exception_handler MC  X86_EX_MC  0
exception_handler XM  X86_EX_XM  0
exception_handler VE  X86_EX_VE  0
exception_handler SE  X86_EX_SE  1
GLOBAL(end_exception_handlers)

GLOBAL(interrupt_handlers)
interrupt_handler timer timer_interrupt_handler
interrupt_handler uart uart_interrupt_handler
interrupt_handler keyboard keyboard_interrupt_handler
#ifdef KTF_ACPICA
interrupt_handler acpi acpi_interrupt_handler
#endif
GLOBAL(end_interrupt_handlers)

.align PAGE_SIZE
ENTRY(syscall_exit)
    POPF

    /* Save exit code to return value register (AX) */
    mov %_ASM_SI, cpu_regs_ax(%_ASM_SP)
    RESTORE_ALL_REGS

    ret
END_FUNC(syscall_exit)

ENTRY(terminate_user_task)
    SWITCH_STACK
    POPF

    movl $-EFAULT, cpu_regs_ax(%_ASM_SP)
    RESTORE_ALL_REGS

    ret
END_FUNC(terminate_user_task)

.align PAGE_SIZE
GLOBAL(usermode_helpers)
ENTRY(enter_usermode)
    /* FIXME: Add 32-bit support */

    /* will be restored on entering back in kernel mode */
    SAVE_ALL_REGS
    PUSHF

    /* Save user stack pointer onto per-cpu */
    mov %_ASM_DX, %gs:(usermode_private)

    /* Now switch stack and address space */
    syscall_to_usermode

    /* SS + SP */
    push $__USER_DS
    push %_ASM_DX

    /* EFLAGS */
    PUSHF

    orl $X86_EFLAGS_IOPL, (%_ASM_SP)

    /* CS + IP */
    push $__USER_CS
    push $usermode_stub
    IRET
END_FUNC(enter_usermode)

ENTRY(syscall_handler_entry)
    SAVE_ALL_REGS include_ax=0
    syscall_from_usermode

    cmp $SYSCALL_EXIT, %_ASM_AX
    jz syscall_exit

    push %_ASM_CX
    push %r11

    mov %_ASM_DI, %_ASM_CX
    mov %_ASM_AX, %_ASM_DI
    call syscall_handler

    pop %r11
    pop %_ASM_CX

    syscall_to_usermode
    RESTORE_ALL_REGS include_ax=0
    SYSRET
END_FUNC(syscall_handler_entry)

ENTRY(sysenter_handler_entry)
    SAVE_ALL_REGS include_ax=0
    syscall_from_usermode

    MASK_USER_FLAGS

    cmp $SYSCALL_EXIT, %_ASM_AX
    jz syscall_exit

    mov %_ASM_DI, %_ASM_DX
    mov %r10, %_ASM_CX
    mov %_ASM_AX, %_ASM_DI
    call syscall_handler

    syscall_to_usermode
    RESTORE_ALL_REGS include_ax=0

    SYSEXIT
END_FUNC(sysenter_handler_entry)

ENTRY(int80_handler_entry)
    SAVE_ALL_REGS include_ax=0
    syscall_from_usermode

    MASK_USER_FLAGS

    cmp $SYSCALL_EXIT, %_ASM_AX
    jz syscall_exit

    mov %_ASM_DI, %_ASM_CX
    mov %_ASM_AX, %_ASM_DI
    call syscall_handler

    syscall_to_usermode
    RESTORE_ALL_REGS include_ax=0
    IRET
END_FUNC(int80_handler_entry)
GLOBAL(end_usermode_helpers)

SECTION(.text.user, "ax", 16)
ENTRY(usermode_stub)
    /* DI: User function to be called
     * SI: Parameters
     * ...
     */
    xchg %_ASM_DI, %_ASM_SI
    call *%_ASM_SI

    /* sys_exit */
    mov %_ASM_AX, %_ASM_SI
    mov $SYSCALL_EXIT, %_ASM_AX
    syscall
END_FUNC(usermode_stub)

SECTION(.data.rmode, "aw", 16)
STRING(rmode_exception_str, Exception happened during real mode call! This is not supported.\n)

SECTION(.text.rmode, "ax", 16)
.code16
ENTRY(rmode_exception)
    puts rmode_exception_str STRING_LEN(rmode_exception_str)

    cli
.Lhalt_loop:
    hlt
    jmp .Lhalt_loop
END_FUNC(rmode_exception)
