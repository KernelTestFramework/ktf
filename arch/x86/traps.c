#include <ktf.h>
#include <lib.h>
#include <segment.h>
#include <console.h>
#include <string.h>
#include <setup.h>
#include <traps.h>
#include <processor.h>

unsigned long ret2kern_sp;
extern void ret2kern_handler(void);

void init_traps(void) {
    set_gate_offset(&idt[X86_EX_DE],  _ul(entry_DE));
    set_gate_offset(&idt[X86_EX_DB],  _ul(entry_DB));
    set_gate_offset(&idt[X86_EX_NMI], _ul(entry_NMI));
    set_gate_offset(&idt[X86_EX_BP],  _ul(entry_BP));
    set_gate_offset(&idt[X86_EX_OF],  _ul(entry_OF));
    set_gate_offset(&idt[X86_EX_BR],  _ul(entry_BR));
    set_gate_offset(&idt[X86_EX_UD],  _ul(entry_UD));
    set_gate_offset(&idt[X86_EX_NM],  _ul(entry_NM));
    set_gate_offset(&idt[X86_EX_CS],  _ul(entry_CS));
    set_gate_offset(&idt[X86_EX_TS],  _ul(entry_TS));
    set_gate_offset(&idt[X86_EX_NP],  _ul(entry_NP));
    set_gate_offset(&idt[X86_EX_SS],  _ul(entry_SS));
    set_gate_offset(&idt[X86_EX_GP],  _ul(entry_GP));
    set_gate_offset(&idt[X86_EX_PF],  _ul(entry_PF));
    set_gate_offset(&idt[X86_EX_MF],  _ul(entry_MF));
    set_gate_offset(&idt[X86_EX_AC],  _ul(entry_AC));
    set_gate_offset(&idt[X86_EX_MC],  _ul(entry_MC));
    set_gate_offset(&idt[X86_EX_XM],  _ul(entry_XM));
    set_gate_offset(&idt[X86_EX_VE],  _ul(entry_VE));
    set_gate_offset(&idt[X86_EX_SE],  _ul(entry_SE));

    tss_df.iopb = sizeof(tss_df);
#if defined (__i386__)
    tss_df.esp0 = _ul(GET_KERN_EM_STACK());
    tss_df.ss   = __KERN_DS;
    tss_df.ds   = __KERN_DS;
    tss_df.es   = __KERN_DS;
    tss_df.fs   = __KERN_DS;
    tss_df.gs   = __KERN_DS;
    tss_df.eip  = _ul(entry_DF);
    tss_df.cs   = __KERN_CS;
    tss_df.cr3  = _ul(l4_pt_entries);

    set_desc_base(&gdt[GDT_TSS_DF], _ul(&tss_df));
    /* FIXME */
#else
    set_gate_offset(&idt[X86_EX_DF], _ul(entry_DF));
    idt[X86_EX_DF].ist = 0x1;
#endif

    barrier();
    lidt(&idt_ptr);

#if defined(__i386__)
    tss.esp0 = _ul(GET_KERN_EX_STACK());
    tss.ss0  = __KERN_DS;
    tss.cr3  = _ul(l4_pt_entries);
#elif defined(__x86_64__)
    tss.rsp0 =   _ul(GET_KERN_EX_STACK());
    tss.ist[0] = _ul(GET_KERN_EM_STACK());
#endif
    tss.iopb = sizeof(tss);

    set_desc_base(&gdt[GDT_TSS], _ul(&tss));

    barrier();
    lgdt(&gdt_ptr);

    barrier();
    ltr(GDT_TSS << 3);

    /* User mode return to kernel handler */
    set_gate_offset(&idt[X86_RET2KERN_INT],  _ul(ret2kern_handler));
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
           regs->_ASM_AX, regs->r8,
           regs->_ASM_BX, regs->r9,
           regs->_ASM_CX, regs->r10,
           regs->_ASM_DX, regs->r11,
           regs->_ASM_SI, regs->r12,
           regs->_ASM_DI, regs->r13,
           regs->_ASM_BP, regs->r14,
           regs->_ASM_SP, regs->r15,
           regs->_ASM_IP);
}

static void dump_control_regs(const struct cpu_regs *regs) {
    printk("CR0=0x%016lx CR2=0x%016lx\n"
           "CR3=0x%016lx CR4=0x%016lx\n"
           "CR8=0x%016lx\n\n",
           read_cr0(), read_cr2(),
           read_cr3(), read_cr4(),
           read_cr8());
}

static void dump_segment_regs(const struct cpu_regs *regs) {
    printk("CURRENT:\n"
           "CS=0x%04x DS=0x%04x SS=0x%04x\n"
           "ES=0x%04x FS=0x%04x GS=0x%04x\n"
           "EXCEPTION:\n"
           "CS=0x%04x SS=0x%04x\n\n",
           read_cs(), read_ds(), read_ss(),
           read_es(), read_fs(), read_gs(),
           regs->cs, regs->ss);
}

static void dump_flags(const struct cpu_regs *regs) {
    printk("RFLAGS=0x%016lx\n\n", regs->_ASM_FLAGS);
}

static void dump_stack(const struct cpu_regs *regs, int words, int lines) {
    unsigned long *sp;
    int i;

    sp = (unsigned long *) regs->_ASM_SP;
    printk("STACK[0x%016p]:", sp);
    for (i = 0; i < (words * lines); i++) {
        if (i > 0 && (_ul(&sp[i]) % PAGE_SIZE) == 0)
            break;
        if ((i % words) == 0)
            printk("\n0x%04x: ", i * (sizeof(unsigned long)));
        printk("%016lx ", sp[i]);
    }
    printk("\n\n");

}

static void dump_regs(const struct cpu_regs *regs) {
    dump_general_regs(regs);
    dump_segment_regs(regs);
    dump_control_regs(regs);
    dump_flags(regs);
    dump_stack(regs, 4, 16);
}

static const char * const exception_names[] = {
    [X86_EX_DE]  = "DE",
    [X86_EX_DB]  = "DB",
    [X86_EX_NMI] = "NMI",
    [X86_EX_BP]  = "BP",
    [X86_EX_OF]  = "OF",
    [X86_EX_BR]  = "BR",
    [X86_EX_UD]  = "UD",
    [X86_EX_NM]  = "NM",
    [X86_EX_DF]  = "DF",
    [X86_EX_CS]  = "CS",
    [X86_EX_TS]  = "TS",
    [X86_EX_NP]  = "NP",
    [X86_EX_SS]  = "SS",
    [X86_EX_GP]  = "GP",
    [X86_EX_PF]  = "PF",
    [X86_EX_SPV] = "SPV",
    [X86_EX_MF]  = "MF",
    [X86_EX_AC]  = "AC",
    [X86_EX_MC]  = "MC",
    [X86_EX_XM]  = "XM",
    [X86_EX_VE]  = "VE",
    [X86_EX_SE]  = "SE",
};

static const char * const tlb_names[] = {
    [X86_EX_SEL_TLB_GDT] = "GDT",
    [X86_EX_SEL_TLB_IDT] = "IDT",
    [X86_EX_SEL_TLB_LDT] = "LDT",
    [X86_EX_SEL_TLB_IDT2] = "IDT",
};

static char *x86_ex_decode_error_code(char *buf, size_t size, uint32_t vector, x86_ex_error_code_t ec) {
    switch (vector) {
    case X86_EX_PF:
       snprintf(buf, size, "%c%c%c%c%c ",
                ec.P ? 'P' : '-',
                ec.W ? 'W' : 'R',
                ec.U ? 'U' : 'S',
                ec.R ? 'R' : '-',
                ec.I ? 'I' : '-');
       break;
    case X86_EX_TS:
    case X86_EX_NP:
    case X86_EX_SS:
    case X86_EX_GP:
    case X86_EX_AC:
       snprintf(buf, size, "%s%s[0x%02x] ",
                ec.E ? "#EXT " : "",
                tlb_names[ec.TLB],
                ec.index);
       break;
    default:
       snprintf(buf, size, "0x%08x ", 0x0);
       break;
    }

    return buf;
}

void do_exception(struct cpu_regs *regs) {
    static char ec_str[32], panic_str[128];

    dump_regs(regs);

    if (has_error_code(regs->vector))
        x86_ex_decode_error_code(ec_str, sizeof(ec_str), regs->vector, regs->error_code);

    snprintf(panic_str, sizeof(panic_str), "#%s %sat IP: 0x%02x:0x%016lx SP: 0x%02x:0x%016lx\n",
             exception_names[regs->vector], ec_str, regs->cs, regs->_ASM_IP, regs->ss, regs->_ASM_SP);

    panic(panic_str);
}
