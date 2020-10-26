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
#include <asm-macros.h>
#include <ktf.h>
#include <lib.h>
#include <processor.h>
#include <segment.h>

extern uint8_t *_boot_stack_ist_top;
extern uint8_t *_boot_stack_df_top;

x86_tss_t __data_init boot_tss __aligned(16);
x86_tss_t __data_init boot_tss_df __aligned(16);

gdt_desc_t __data_init boot_gdt[NR_BOOT_GDT_ENTRIES] __aligned(16) = {
    /* clang-format off */
    [GDT_NULL].desc      = GDT_ENTRY(0x0, 0x0, 0x0),
    [GDT_KERN_CS32].desc = GDT_ENTRY(DESC_FLAGS(GR, SZ, P, DPL0, S, CODE, R, A), 0x0, 0xfffff),
    [GDT_KERN_DS32].desc = GDT_ENTRY(DESC_FLAGS(GR, SZ, P, DPL0, S, DATA, W, A), 0x0, 0xfffff),
    [GDT_KERN_CS64].desc = GDT_ENTRY(DESC_FLAGS(GR,  L, P, DPL0, S, CODE, R, A), 0x0, 0x00000),
    /* clang-format on */
};

gdt_ptr_t __data_init boot_gdt_ptr = {
    .size = sizeof(boot_gdt) - 1,
    .addr = _ul(&boot_gdt),
};

idt_entry_t __data_init boot_idt[256];

idt_ptr_t boot_idt_ptr __data_init = {
    .size = sizeof(boot_idt) - 1,
    .addr = _ul(&boot_idt),
};

static void __text_init init_boot_tss(void) {
#if defined(__i386__)
    boot_tss_df.iopb = sizeof(boot_tss_df);
    boot_tss_df.esp0 = _ul(_boot_stack_df_top);
    boot_tss_df.ss = __KERN_DS;
    boot_tss_df.ds = __KERN_DS;
    boot_tss_df.es = __KERN_DS;
    boot_tss_df.fs = __KERN_DS;
    boot_tss_df.gs = __KERN_DS;
    boot_tss_df.eip = _ul(entry_DF);
    boot_tss_df.cs = __KERN_CS;
    boot_tss_df.cr3 = read_cr3();

    /* Assign identity mapping of the tss_df, because GDT has only 32-bit base. */
    boot_gdt[GDT_BOOT_TSS_DF].desc =
        GDT_ENTRY(DESC_FLAGS(SZ, P, CODE, A), _ul(&boot_tss_df), sizeof(boot_tss_df) - 1);

    /* FIXME */
    boot_tss.esp0 = _ul(_boot_stack_ist_top);
    boot_tss.ss0 = __KERN_DS;
    boot_tss.cr3 = read_cr3();
#elif defined(__x86_64__)
    boot_tss.rsp0 = _ul(_boot_stack_ist_top);
    boot_tss.ist[0] = _ul(_boot_stack_df_top);
#endif
    boot_tss.iopb = sizeof(boot_tss);

    /* Assign identity mapping of the tss, because GDT has only 32-bit base. */
    boot_gdt[GDT_BOOT_TSS].desc =
        GDT_ENTRY(DESC_FLAGS(SZ, P, CODE, A), _ul(&boot_tss), sizeof(boot_tss) - 1);
#if defined(__x86_64__)
    boot_gdt[GDT_BOOT_TSS + 1].desc = GDT_ENTRY(0x0, 0x0, 0x0);
#endif

    barrier();
    ltr(GDT_BOOT_TSS << 3);
}

void __text_init init_boot_traps(void) {
    /* clang-format off */
    set_intr_gate(&boot_idt[X86_EX_DE],  __KERN_CS, _ul(entry_DE),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&boot_idt[X86_EX_DB],  __KERN_CS, _ul(entry_DB),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&boot_idt[X86_EX_NMI], __KERN_CS, _ul(entry_NMI), GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&boot_idt[X86_EX_BP],  __KERN_CS, _ul(entry_BP),  GATE_DPL3, GATE_PRESENT, 0);
    set_intr_gate(&boot_idt[X86_EX_OF],  __KERN_CS, _ul(entry_OF),  GATE_DPL3, GATE_PRESENT, 0);
    set_intr_gate(&boot_idt[X86_EX_BR],  __KERN_CS, _ul(entry_BR),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&boot_idt[X86_EX_UD],  __KERN_CS, _ul(entry_UD),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&boot_idt[X86_EX_NM],  __KERN_CS, _ul(entry_NM),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&boot_idt[X86_EX_CS],  __KERN_CS, _ul(entry_CS),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&boot_idt[X86_EX_TS],  __KERN_CS, _ul(entry_TS),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&boot_idt[X86_EX_NP],  __KERN_CS, _ul(entry_NP),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&boot_idt[X86_EX_SS],  __KERN_CS, _ul(entry_SS),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&boot_idt[X86_EX_GP],  __KERN_CS, _ul(entry_GP),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&boot_idt[X86_EX_PF],  __KERN_CS, _ul(entry_PF),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&boot_idt[X86_EX_MF],  __KERN_CS, _ul(entry_MF),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&boot_idt[X86_EX_AC],  __KERN_CS, _ul(entry_AC),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&boot_idt[X86_EX_MC],  __KERN_CS, _ul(entry_MC),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&boot_idt[X86_EX_XM],  __KERN_CS, _ul(entry_XM),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&boot_idt[X86_EX_VE],  __KERN_CS, _ul(entry_VE),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&boot_idt[X86_EX_SE],  __KERN_CS, _ul(entry_SE),  GATE_DPL0, GATE_PRESENT, 0);
#if defined(__x86_64__)
    set_intr_gate(&boot_idt[X86_EX_DF],  __KERN_CS, _ul(entry_DF),  GATE_DPL0, GATE_PRESENT, 1);
#endif
    /* clang-format on */

    barrier();
    lidt(&boot_idt_ptr);

    init_boot_tss();
}