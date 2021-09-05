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
#ifndef KTF_ASM_MACROS_H
#define KTF_ASM_MACROS_H

#if defined(__i386__)
#define _ASM_REG(reg) e##reg
#elif defined(__x86_64__)
#define _ASM_REG(reg) r##reg
#else
#define _ASM_REG(reg) reg
#endif

#define _ASM_AX    _ASM_REG(ax)
#define _ASM_BX    _ASM_REG(bx)
#define _ASM_CX    _ASM_REG(cx)
#define _ASM_DX    _ASM_REG(dx)
#define _ASM_SI    _ASM_REG(si)
#define _ASM_DI    _ASM_REG(di)
#define _ASM_BP    _ASM_REG(bp)
#define _ASM_SP    _ASM_REG(sp)
#define _ASM_IP    _ASM_REG(ip)
#define _ASM_FLAGS _ASM_REG(flags)

#define PT_PADDR(table, shift) (((.- (table)) / PTE_SIZE) << (shift))

#define PUSHSECTION(name) ".pushsection " STR(name) " ;\n"
#define POPSECTION        ".popsection;\n"

#ifdef __ASSEMBLY__

/* clang-format off */
.macro putc val
  movw (com_ports), %dx;
  movb $\val, %al;
  outb %al, %dx;
.endm

.macro puts addr len
    movw (com_ports), %dx
    mov $\addr, %si
    mov $\len, %cx

    cld
    rep outsb
.endm

.macro SAVE_REGS_32
    push %eax
    push %ebx
    push %ecx
    push %edx
    push %esi
    push %edi
    push %ebp
.endm

.macro RESTORE_REGS_32
    pop %ebp
    pop %edi
    pop %esi
    pop %edx
    pop %ecx
    pop %ebx
    pop %eax
.endm

.macro SAVE_REGS
    push %_ASM_AX
    push %_ASM_BX
    push %_ASM_CX
    push %_ASM_DX
    push %_ASM_SI
    push %_ASM_DI
    push %_ASM_BP
#if defined(__x86_64__)
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
#endif
.endm

.macro RESTORE_REGS
#if defined(__x86_64__)
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
#endif
    pop %_ASM_BP
    pop %_ASM_DI
    pop %_ASM_SI
    pop %_ASM_DX
    pop %_ASM_CX
    pop %_ASM_BX
    pop %_ASM_AX
.endm

.macro PUSHF
#if defined(__x86_64__)
    pushfq
#else
    pushf
#endif
.endm

.macro POPF
#if defined(__x86_64__)
    popfq
#else
    popf
#endif
.endm

.macro IRET
#if defined(__x86_64__)
    iretq
#else
    iret
#endif
.endm

#define GLOBAL(name) \
    .global name;    \
name:

#define ENTRY(name) \
    .align 16;      \
    GLOBAL(name)

#define SECTION(name, flags, alignment) \
    .section name, flags;               \
    .align alignment

#define SIZE(name) \
    .size name, (. - name);

#define END_OBJECT(name)    \
    .type name, STT_OBJECT; \
    SIZE(name)

#define END_FUNC(name)     \
    .type name, STT_FUNC;  \
    SIZE(name)

#define STRING(name, text) \
name:;                     \
    .asciz #text;          \
name ## _end:

#define STRING_LEN(name) (name ## _end - name)

#define ELF_NOTE(name, type, size, addr) \
    .section .note.name, "a";            \
    .align 4;                            \
    .long 2f - 1f; /* namesz */          \
    .long 4f - 3f; /* descsz */          \
    .long type;    /* type   */          \
1:.asciz #name;    /* name   */          \
2:.align 4;                              \
3:size addr;       /* desc   */          \
4:.align 4;
/* clang-format on */

#endif

#endif /* KTF_ASM_MACROS_H */
