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
#include <page.h>

.code64
SECTION(.text.rmode, "ax", 16)
ENTRY(_long_to_real)
    /* Save current GDT and IDT in low memory area */
    mov   %rdi, _saved_gdt_ptr
    mov   %rsi, _saved_idt_ptr

    /* Save current flags register */
    pushfq

    /* Disable interrupts. If we get NMI or MC, hope we get back here */
    cli

    /* Save control registers context */
    mov   %cr0, %rax
    mov   %rax, _saved_cr0

    mov   %cr3, %rax
    mov   %rax, _saved_cr3

    mov   %cr4, %rax
    mov   %rax, _saved_cr4

    /* Save EFER MSR */
    xor   %rax, %rax
    xor   %rdx, %rdx
    movl  $MSR_EFER, %ecx
    rdmsr

    shl   $32, %rdx
    or    %rdx, %rax
    mov   %rax, _saved_efer

    /* Save segment selector registers */
    movw  %ds, %ax
    movw  %ax, _saved_ds

    movw  %es, %ax
    movw  %ax, _saved_es

    movw  %gs, %ax
    movw  %ax, _saved_gs

    movw  %fs, %ax
    movw  %ax, _saved_fs

    movw  %ss, %ax
    movw  %ax, _saved_ss

    /* Save stack pointer */
    mov   %rsp, _saved_sp

    push  $__KERN_CS32
    push  $.Lprot_mode
    lretq
END_FUNC(_long_to_real)

.code32
.align 16
.Lprot_mode:
    /* Load real mode accessible GDT and IDT */
    lgdt  rmode_gdt_ptr
    lidt  rmode_idt_ptr

    /* Disable LME in EFER */
    movl  $MSR_EFER, %ecx
    rdmsr
    and   $~EFER_LME, %eax
    wrmsr

    /* Disable paging to enter protected mode */
    mov   %cr0, %eax
    and   $~(X86_CR0_PG | X86_CR0_WP), %eax
    mov   %eax, %cr0

    /* Use protected mode data selector */
    mov   $__KERN_DS32, %eax
    mov   %eax, %ds

    /* Use real mode accessible stack */
    mov   rmode_stack, %esp

    /* Use 16-bit protected mode segment selectors */
    mov   $__KERN_DS16, %eax
    mov   %eax, %ds
    mov   %eax, %es
    mov   %eax, %gs
    mov   %eax, %fs
    mov   %eax, %ss

    /* Jump to real mode */
    ljmp  $__KERN_CS16, $.Lreal_mode

.code16
.align 16
.Lreal_mode:
    /* Clear CR3 register */
    xor   %eax, %eax
    mov   %eax, %cr3

    /* Disable protected mode and caching */
    mov   %cr0, %eax
    push  %eax
    and   $~X86_CR0_PE, %eax
    or    $(X86_CR0_CD | X86_CR0_NW), %eax
    wbinvd
    mov   %eax, %cr0

    /* Set CS to first segment. It should cover the .text.rmode section */
    ljmp  $0x0, $.Lset_rmode_cs
.Lset_rmode_cs:

    /* Set other segment selectors to the first segment.
     * It should cover the .bss.rmode and first pages of
     * .data.rmode sections
     */
    mov   %cs, %ax
    mov   %ax, %ds
    mov   %ax, %es
    mov   %ax, %gs
    mov   %ax, %fs
    mov   %ax, %ss

    /* CALL */

    /* Restore protected mode and caching */
    pop   %eax
    mov   %eax, %cr0

    /* Reload GDT and IDT pointers - just in case... */
    lgdt  rmode_gdt_ptr
    lidt  rmode_idt_ptr

    /* Use protected mode data selector */
    mov   $__KERN_DS32, %eax
    mov   %eax, %ds
    mov   %eax, %es
    mov   %eax, %gs
    mov   %eax, %fs
    mov   %eax, %ss

    ljmp  $__KERN_CS32, $.Lret_to_prot_mode

.code32
.align 16
.Lret_to_prot_mode:
    mov   %cr4, %eax
    or    $(X86_CR4_PAE | X86_CR4_PSE), %eax
    mov   %eax, %cr4

    /* Load temporarily real mode page tables.
     * This is needed to enable long mode.
     */
    mov   $rm_l4_pt_entries, %eax
    mov   %eax, %cr3

    /* Restore EFER MSR */
    movl  _saved_efer_high, %edx
    movl  _saved_efer, %eax
    movl  $MSR_EFER, %ecx
    wrmsr

    /* Re-enable paging to activate long mode */
    mov   %cr0, %eax
    or    $(X86_CR0_PG | X86_CR0_WP), %eax
    mov   %eax, %cr0

    ljmp  $__KERN_CS64, $.Lret_to_long_mode

.code64
.align 16
.Lret_to_long_mode:
    /* Restore control registers */
    mov   _saved_cr0, %rax
    mov   %rax, %cr0

    /* Restore original page tables */
    mov   _saved_cr3, %rax
    mov   %rax, %cr3

    mov   _saved_cr4, %rax
    mov   %rax, %cr4

    /* Restore GDT and IDT descriptors */
    mov   _saved_gdt_ptr, %rax
    lgdt  (%rax)
    mov   _saved_idt_ptr, %rax
    lidt  (%rax)

    /* Restore segment selector registers */
    mov   _saved_ds, %rax
    mov   %rax, %ds

    mov   _saved_es, %rax
    mov   %rax, %es

    mov   _saved_fs, %rax
    mov   %rax, %fs

    mov   _saved_gs, %rax
    mov   %rax, %gs

    mov   _saved_ss, %rax
    mov   %rax, %ss

    /* Restore stack pointer */
    mov   _saved_sp, %rsp

    /* Restore flags register */
    popfq

    ret

/* Data section allocations */

SECTION(.bss.rmode, "aw", 16)
GLOBAL(_saved_cr0)
    .quad 0x0

GLOBAL(_saved_cr3)
    .quad 0x0

GLOBAL(_saved_cr4)
    .quad 0x0

GLOBAL(_saved_sp)
    .quad 0x0

GLOBAL(_saved_efer)
    .long 0x0
GLOBAL(_saved_efer_high)
    .long 0x0

GLOBAL(_saved_ds)
    .word 0x0

GLOBAL(_saved_es)
    .word 0x0

GLOBAL(_saved_fs)
    .word 0x0

GLOBAL(_saved_gs)
    .word 0x0

GLOBAL(_saved_ss)
    .word 0x0

.align 16
GLOBAL(_saved_gdt_ptr)
    .quad 0x0

.align 16
GLOBAL(_saved_idt_ptr)
    .quad 0x0

.align PAGE_SIZE
rmode_stack_top:
    .skip PAGE_SIZE
GLOBAL(rmode_stack)

.align PAGE_SIZE
rmode_stack_ist:
    .skip PAGE_SIZE
GLOBAL(rmode_stack_ist_top)

.align PAGE_SIZE
rmode_stack_df:
    .skip PAGE_SIZE
GLOBAL(rmode_stack_df_top)

/* Temporary real-mode-accessible page tables */

SECTION(.data.rmode, "aw", PAGE_SIZE)
GLOBAL(rm_l1_pt_entries)
    .rept L1_PT_ENTRIES / 2 /* Cover only first 1MB of memory */
    .long PT_PADDR(rm_l1_pt_entries, L1_PT_SHIFT) + L1_PROT, 0
    .endr
END_OBJECT(rm_l1_pt_entries)

.align PAGE_SIZE
GLOBAL(rm_l2_pt_entries)
    .long rm_l1_pt_entries + L2_PROT, 0
END_OBJECT(rm_l2_pt_entries)

.align PAGE_SIZE
GLOBAL(rm_l3_pt_entries)
    .long rm_l2_pt_entries + L3_PROT, 0
END_OBJECT(rm_l3_pt_entries)

#if defined(__x86_64__)
.align PAGE_SIZE
GLOBAL(rm_l4_pt_entries)
    .long rm_l3_pt_entries + L4_PROT, 0
END_OBJECT(rm_l4_pt_entries)
#endif
