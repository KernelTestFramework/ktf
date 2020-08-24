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
#include <console.h>
#include <ktf.h>
#include <lib.h>
#include <percpu.h>
#include <processor.h>
#include <segment.h>
#include <setup.h>
#include <string.h>
#include <symbols.h>
#include <traps.h>

#include <mm/vmm.h>

extern void ret2kern_handler(void);

static void init_tss(percpu_t *percpu) {
#if defined(__i386__)
    percpu->tss_df.iopb = sizeof(percpu->tss_df);
    percpu->tss_df.esp0 = _ul(get_free_page_top(GFP_KERNEL));
    percpu->tss_df.ss = __KERN_DS;
    percpu->tss_df.ds = __KERN_DS;
    percpu->tss_df.es = __KERN_DS;
    percpu->tss_df.fs = __KERN_DS;
    percpu->tss_df.gs = __KERN_DS;
    percpu->tss_df.eip = _ul(entry_DF);
    percpu->tss_df.cs = __KERN_CS;
    percpu->tss_df.cr3 = _ul(cr3.reg);

    /* Assign identity mapping of the tss_df, because GDT has only 32-bit base. */
    percpu->gdt[GDT_TSS_DF].desc =
        GDT_ENTRY(DESC_FLAGS(SZ, P, CODE, A), virt_to_paddr(&percpu->tss_df),
                  sizeof(percpu->tss_df) - 1);

    /* FIXME */
    percpu->tss.esp0 = _ul(get_free_page_top(GFP_KERNEL));
    percpu->tss.ss0 = __KERN_DS;
    percpu->tss.cr3 = _ul(cr3.reg);
#elif defined(__x86_64__)
    percpu->tss.rsp0 = _ul(get_free_page_top(GFP_KERNEL));
    percpu->tss.ist[0] = _ul(get_free_page_top(GFP_KERNEL));
#endif
    percpu->tss.iopb = sizeof(percpu->tss);

    /* Assign identity mapping of the tss, because GDT has only 32-bit base. */
    percpu->gdt[GDT_TSS].desc = GDT_ENTRY(
        DESC_FLAGS(SZ, P, CODE, A), virt_to_paddr(&percpu->tss), sizeof(percpu->tss) - 1);
#if defined(__x86_64__)
    percpu->gdt[GDT_TSS + 1].desc = GDT_ENTRY(0x0, 0x0, 0x0);
#endif

    barrier();
    ltr(GDT_TSS << 3);
}

static void init_gdt(percpu_t *percpu) {
    /* clang-format off */
    percpu->gdt[GDT_NULL].desc      = GDT_ENTRY(0x0, 0x0, 0x0);
    percpu->gdt[GDT_KERN_CS32].desc = GDT_ENTRY(DESC_FLAGS(GR, SZ, P, DPL0, S, CODE, R, A), 0x0, 0xfffff);
    percpu->gdt[GDT_KERN_DS32].desc = GDT_ENTRY(DESC_FLAGS(GR, SZ, P, DPL0, S, DATA, W, A), 0x0, 0xfffff);
    percpu->gdt[GDT_KERN_CS64].desc = GDT_ENTRY(DESC_FLAGS(GR,  L, P, DPL0, S, CODE, R, A), 0x0, 0x00000);

    percpu->gdt[GDT_USER_CS32].desc = GDT_ENTRY(DESC_FLAGS(GR, SZ, P, DPL3, S, CODE, R, A), 0x0, 0xfffff);
    percpu->gdt[GDT_USER_DS32].desc = GDT_ENTRY(DESC_FLAGS(GR, SZ, P, DPL3, S, DATA, W, A), 0x0, 0xfffff);
    percpu->gdt[GDT_USER_CS64].desc = GDT_ENTRY(DESC_FLAGS(GR,  L, P, DPL3, S, CODE, R, A), 0x0, 0x00000);

    /* Assign identity mapping of the percpu, because GDT has only 32-bit base. */
    percpu->gdt[GDT_PERCPU].desc = GDT_ENTRY(DESC_FLAGS(GR, L, P, DPL3, S, CODE, R, A), virt_to_paddr(percpu), PAGE_SIZE);
    /* clang-format on */

    percpu->gdt_ptr.size = sizeof(percpu->gdt) - 1;
    percpu->gdt_ptr.addr = _ul(&percpu->gdt);

    barrier();
    lgdt(&percpu->gdt_ptr);

    write_gs(GDT_PERCPU << 3);

    init_tss(percpu);
}

void init_traps(unsigned int cpu) {
    percpu_t *percpu = get_percpu_page(cpu);

    BUG_ON(!percpu);

    percpu->idt = get_free_page(GFP_KERNEL);
    BUG_ON(!percpu->idt);

    percpu->idt_ptr.size = (sizeof(percpu->idt) * MAX_INT) - 1;
    percpu->idt_ptr.addr = _ul(percpu->idt);

    /* clang-format off */
    set_intr_gate(&percpu->idt[X86_EX_DE],  __KERN_CS, _ul(entry_DE),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&percpu->idt[X86_EX_DB],  __KERN_CS, _ul(entry_DB),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&percpu->idt[X86_EX_NMI], __KERN_CS, _ul(entry_NMI), GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&percpu->idt[X86_EX_BP],  __KERN_CS, _ul(entry_BP),  GATE_DPL3, GATE_PRESENT, 0);
    set_intr_gate(&percpu->idt[X86_EX_OF],  __KERN_CS, _ul(entry_OF),  GATE_DPL3, GATE_PRESENT, 0);
    set_intr_gate(&percpu->idt[X86_EX_BR],  __KERN_CS, _ul(entry_BR),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&percpu->idt[X86_EX_UD],  __KERN_CS, _ul(entry_UD),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&percpu->idt[X86_EX_NM],  __KERN_CS, _ul(entry_NM),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&percpu->idt[X86_EX_CS],  __KERN_CS, _ul(entry_CS),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&percpu->idt[X86_EX_TS],  __KERN_CS, _ul(entry_TS),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&percpu->idt[X86_EX_NP],  __KERN_CS, _ul(entry_NP),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&percpu->idt[X86_EX_SS],  __KERN_CS, _ul(entry_SS),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&percpu->idt[X86_EX_GP],  __KERN_CS, _ul(entry_GP),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&percpu->idt[X86_EX_PF],  __KERN_CS, _ul(entry_PF),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&percpu->idt[X86_EX_MF],  __KERN_CS, _ul(entry_MF),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&percpu->idt[X86_EX_AC],  __KERN_CS, _ul(entry_AC),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&percpu->idt[X86_EX_MC],  __KERN_CS, _ul(entry_MC),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&percpu->idt[X86_EX_XM],  __KERN_CS, _ul(entry_XM),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&percpu->idt[X86_EX_VE],  __KERN_CS, _ul(entry_VE),  GATE_DPL0, GATE_PRESENT, 0);
    set_intr_gate(&percpu->idt[X86_EX_SE],  __KERN_CS, _ul(entry_SE),  GATE_DPL0, GATE_PRESENT, 0);
#if defined(__x86_64__)
    set_intr_gate(&percpu->idt[X86_EX_DF],  __KERN_CS, _ul(entry_DF),  GATE_DPL0, GATE_PRESENT, 1);
#endif

    /* User mode return to kernel handler */
    set_intr_gate(&percpu->idt[X86_RET2KERN_INT], __KERN_CS, _ul(ret2kern_handler), GATE_DPL3, GATE_PRESENT, 0);
    /* clang-format on */

    barrier();
    lidt(&percpu->idt_ptr);

    init_gdt(percpu);

    wrmsr(MSR_TSC_AUX, cpu);
}

static void dump_general_regs(const struct cpu_regs *regs) {
    printk("RAX=0x%016lx  R8=0x%016lx\n"
           "RBX=0x%016lx  R9=0x%016lx\n"
           "RCX=0x%016lx R10=0x%016lx\n"
           "RDX=0x%016lx R11=0x%016lx\n"
           "RSI=0x%016lx R12=0x%016lx\n"
           "RDI=0x%016lx R13=0x%016lx\n"
           "RBP=0x%016lx R14=0x%016lx\n"
           "RSP=0x%016lx R15=0x%016lx\n"
           "\nRIP=0x%016lx\n\n",
           regs->_ASM_AX, regs->r8, regs->_ASM_BX, regs->r9, regs->_ASM_CX, regs->r10,
           regs->_ASM_DX, regs->r11, regs->_ASM_SI, regs->r12, regs->_ASM_DI, regs->r13,
           regs->_ASM_BP, regs->r14, regs->_ASM_SP, regs->r15, regs->_ASM_IP);
}

static void dump_control_regs(const struct cpu_regs *regs) {
    printk("CR0=0x%016lx CR2=0x%016lx\n"
           "CR3=0x%016lx CR4=0x%016lx\n"
           "CR8=0x%016lx\n\n",
           read_cr0(), read_cr2(), read_cr3(), read_cr4(), read_cr8());
}

static void dump_segment_regs(const struct cpu_regs *regs) {
    printk("CURRENT:\n"
           "CS=0x%04lx DS=0x%04lx SS=0x%04lx\n"
           "ES=0x%04lx FS=0x%04lx GS=0x%04lx\n"
           "EXCEPTION:\n"
           "CS=0x%04x SS=0x%04x\n\n",
           read_cs(), read_ds(), read_ss(), read_es(), read_fs(), read_gs(), regs->cs,
           regs->ss);
}

static void dump_flags(const struct cpu_regs *regs) {
    printk("RFLAGS=0x%016lx\n\n", regs->_ASM_FLAGS);
}

static void dump_stack(const struct cpu_regs *regs, unsigned words) {
    unsigned long *sp = (unsigned long *) regs->_ASM_SP;

    printk("STACK[0x%016p]:", sp);
    for (unsigned i = 0; i == 0 || (_ul(&sp[i]) % PAGE_SIZE_2M); i++) {
        if ((i % words) == 0)
            printk("\n0x%04lx: ", i * (sizeof(unsigned long)));
        printk("%016lx ", sp[i]);
    }
    printk("\n\n");
}

static void dump_regs(const struct cpu_regs *regs) {
    dump_general_regs(regs);
    dump_segment_regs(regs);
    dump_control_regs(regs);
    dump_flags(regs);
    dump_stack(regs, 4);
}

static const char *const exception_names[] = {
    [X86_EX_DE] = "DE", [X86_EX_DB] = "DB", [X86_EX_NMI] = "NMI", [X86_EX_BP] = "BP",
    [X86_EX_OF] = "OF", [X86_EX_BR] = "BR", [X86_EX_UD] = "UD",   [X86_EX_NM] = "NM",
    [X86_EX_DF] = "DF", [X86_EX_CS] = "CS", [X86_EX_TS] = "TS",   [X86_EX_NP] = "NP",
    [X86_EX_SS] = "SS", [X86_EX_GP] = "GP", [X86_EX_PF] = "PF",   [X86_EX_SPV] = "SPV",
    [X86_EX_MF] = "MF", [X86_EX_AC] = "AC", [X86_EX_MC] = "MC",   [X86_EX_XM] = "XM",
    [X86_EX_VE] = "VE", [X86_EX_SE] = "SE",
};

static const char *const tlb_names[] = {
    [X86_EX_SEL_TLB_GDT] = "GDT",
    [X86_EX_SEL_TLB_IDT] = "IDT",
    [X86_EX_SEL_TLB_LDT] = "LDT",
    [X86_EX_SEL_TLB_IDT2] = "IDT",
};

static char *x86_ex_decode_error_code(char *buf, size_t size, uint32_t vector,
                                      x86_ex_error_code_t ec) {
    switch (vector) {
    case X86_EX_PF:
        snprintf(buf, size, "%c%c%c%c%c ", ec.P ? 'P' : '-', ec.W ? 'W' : 'R',
                 ec.U ? 'U' : 'S', ec.R ? 'R' : '-', ec.I ? 'I' : '-');
        break;
    case X86_EX_TS:
    case X86_EX_NP:
    case X86_EX_SS:
    case X86_EX_GP:
    case X86_EX_AC:
        snprintf(buf, size, "%s%s[0x%02x] ", ec.E ? "#EXT " : "", tlb_names[ec.TLB],
                 ec.index);
        break;
    default:
        snprintf(buf, size, "0x%08x ", 0x0);
        break;
    }

    return buf;
}

void print_callstack(const void *sp, const void *ip) {
    unsigned long *_sp = (unsigned long *) (_ul(sp) & ~0x7UL);

    printk("CALLSTACK:\n");
    print_symbol(ip);
    for (int i = 0; _ul(&_sp[i]) % PAGE_SIZE_2M; i++)
        print_symbol(_ptr(_sp[i]));
    printk("\n");
}

void do_exception(struct cpu_regs *regs) {
    static char ec_str[32], panic_str[128];

    dump_regs(regs);
    print_callstack(_ptr(regs->_ASM_SP), _ptr(regs->_ASM_IP));

    if (has_error_code(regs->vector))
        x86_ex_decode_error_code(ec_str, sizeof(ec_str), regs->vector, regs->error_code);

    snprintf(panic_str, sizeof(panic_str),
             "#%s %sat IP: 0x%02x:0x%016lx SP: 0x%02x:0x%016lx\n",
             exception_names[regs->vector], ec_str, regs->cs, regs->_ASM_IP, regs->ss,
             regs->_ASM_SP);

    panic(panic_str);
}
