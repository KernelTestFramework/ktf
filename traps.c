#include <ktf.h>
#include <desc.h>
#include <console.h>
#include <string.h>
#include <processor.h>

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

    barrier();
    asm volatile ("lidt %0" :: "m" (idt_ptr));

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
       snprintf(buf, size, "%c%c%c%c%c",
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
       snprintf(buf, size, "%s%s[0x%02x]",
                ec.E ? "#EXT " : "",
                tlb_names[ec.TLB],
                ec.index << 3);
       break;
    default:
       snprintf(buf, size, "0x%08x", 0x0);
       break;
    }

    return buf;
}

void do_exception(struct cpu_regs *regs) {
    if (has_error_code(regs->vector)) {
        static char ec_str[32];

        x86_ex_decode_error_code(ec_str, sizeof(ec_str), regs->vector, regs->error_code);
        panic("#%s (error code: %s)\n", exception_names[regs->vector], ec_str);
    }
    else
        panic("#%s\n", exception_names[regs->vector]);
}
