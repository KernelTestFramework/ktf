/*
 * Copyright © 2020 Amazon.com, Inc. or its affiliates.
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
#include <asm-macros.h>
#include <processor.h>
#include <segment.h>

#define BOOT_STACK_SIZE 0x1000

.code32
SECTION(.text.init, "ax", 16)
GLOBAL(_start)
    /* Save multiboot bootloader magic */
    mov %eax, %edi
    mov %ebx, %esi

    mov   %cs, %ax
    mov   %ax, %ds

    lgdt  boot_gdt_ptr

    xor   %ax, %ax
    inc   %ax
    lmsw  %ax

    ljmp  $__KERN_CS32, $.Lprot_mode

.Lprot_mode:
    mov   $__KERN_DS32, %eax
    mov   %eax, %ds
    mov   %eax, %es
    mov   %eax, %gs
    mov   %eax, %fs
    mov   %eax, %ss
    mov   $_boot_stack_top, %esp
    mov   %esp, %ebp

    mov   %cr4, %eax
    or    $(X86_CR4_PAE | X86_CR4_PSE), %eax
    mov   %eax, %cr4

    mov   $l4_pt_entries, %eax
    mov   %eax, %cr3

    /* Enable long mode */
    movl  $MSR_EFER, %ecx
    rdmsr
    or    $EFER_LME, %eax
    wrmsr

    /* Activate long mode: enable paging */
    mov   %cr0, %eax
    or    $(X86_CR0_PG | X86_CR0_WP), %eax
    mov   %eax, %cr0

    /* clear prefetch and jump to 64bit code */
    ljmp $__KERN_CS64, $.Llong_mode

.code64
.Llong_mode:
    xor %rax, %rax
    mov %rax, %ds
    mov %rax, %es
    mov %rax, %fs
    mov %rax, %gs
    mov %rax, %ss

    push $X86_EFLAGS_MBS
    popf

    cld

    jmp kernel_start

    ud2

SECTION(.bss.init, "aw", 16)
_boot_stack:
    .skip BOOT_STACK_SIZE
GLOBAL(_boot_stack_top)

_boot_stack_ist:
    .skip BOOT_STACK_SIZE
GLOBAL(_boot_stack_ist_top)

_boot_stack_df:
    .skip BOOT_STACK_SIZE
GLOBAL(_boot_stack_df_top)

#define XEN_ELFNOTE_PHYS32_ENTRY  18
ELF_NOTE(Xen, XEN_ELFNOTE_PHYS32_ENTRY, .long, _start)
