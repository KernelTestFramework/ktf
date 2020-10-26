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
#ifndef KTF_PROCESSOR_H
#define KTF_PROCESSOR_H

#include <compiler.h>

/*
 * EFLAGS bits.
 */
#define X86_EFLAGS_CF   0x00000001 /* Carry Flag                */
#define X86_EFLAGS_MBS  0x00000002 /* Resvd bit                 */
#define X86_EFLAGS_PF   0x00000004 /* Parity Flag               */
#define X86_EFLAGS_AF   0x00000010 /* Auxillary carry Flag      */
#define X86_EFLAGS_ZF   0x00000040 /* Zero Flag                 */
#define X86_EFLAGS_SF   0x00000080 /* Sign Flag                 */
#define X86_EFLAGS_TF   0x00000100 /* Trap Flag                 */
#define X86_EFLAGS_IF   0x00000200 /* Interrupt Flag            */
#define X86_EFLAGS_DF   0x00000400 /* Direction Flag            */
#define X86_EFLAGS_OF   0x00000800 /* Overflow Flag             */
#define X86_EFLAGS_IOPL 0x00003000 /* IOPL mask                 */
#define X86_EFLAGS_NT   0x00004000 /* Nested Task               */
#define X86_EFLAGS_RF   0x00010000 /* Resume Flag               */
#define X86_EFLAGS_VM   0x00020000 /* Virtual Mode              */
#define X86_EFLAGS_AC   0x00040000 /* Alignment Check           */
#define X86_EFLAGS_VIF  0x00080000 /* Virtual Interrupt Flag    */
#define X86_EFLAGS_VIP  0x00100000 /* Virtual Interrupt Pending */
#define X86_EFLAGS_ID   0x00200000 /* CPUID detection flag      */

/*
 * CPU flags in CR0.
 */
#define X86_CR0_PE 0x00000001 /* Enable Protected Mode    (RW) */
#define X86_CR0_MP 0x00000002 /* Monitor Coprocessor      (RW) */
#define X86_CR0_EM 0x00000004 /* Require FPU Emulation    (RO) */
#define X86_CR0_TS 0x00000008 /* Task Switched            (RW) */
#define X86_CR0_ET 0x00000010 /* Extension type           (RO) */
#define X86_CR0_NE 0x00000020 /* Numeric Error Reporting  (RW) */
#define X86_CR0_WP 0x00010000 /* Supervisor Write Protect (RW) */
#define X86_CR0_AM 0x00040000 /* Alignment Checking       (RW) */
#define X86_CR0_NW 0x20000000 /* Not Write-Through        (RW) */
#define X86_CR0_CD 0x40000000 /* Cache Disable            (RW) */
#define X86_CR0_PG 0x80000000 /* Paging                   (RW) */

/*
 * CPU features in CR4.
 */
#define X86_CR4_VME        0x00000001 /* VM86 extensions                */
#define X86_CR4_PVI        0x00000002 /* Virtual interrupts flag        */
#define X86_CR4_TSD        0x00000004 /* Disable time stamp at ipl 3    */
#define X86_CR4_DE         0x00000008 /* Debugging extensions           */
#define X86_CR4_PSE        0x00000010 /* Page size extensions           */
#define X86_CR4_PAE        0x00000020 /* Physical address extensions    */
#define X86_CR4_MCE        0x00000040 /* Machine check                  */
#define X86_CR4_PGE        0x00000080 /* Global pages                   */
#define X86_CR4_PCE        0x00000100 /* Performance counters at ipl 3  */
#define X86_CR4_OSFXSR     0x00000200 /* Fast FPU save and restore      */
#define X86_CR4_OSXMMEXCPT 0x00000400 /* Unmasked SSE exceptions        */
#define X86_CR4_UMIP       0x00000800 /* UMIP                           */
#define X86_CR4_VMXE       0x00002000 /* VMX                            */
#define X86_CR4_SMXE       0x00004000 /* SMX                            */
#define X86_CR4_FSGSBASE   0x00010000 /* {rd,wr}{fs,gs}base             */
#define X86_CR4_PCIDE      0x00020000 /* PCID                           */
#define X86_CR4_OSXSAVE    0x00040000 /* XSAVE/XRSTOR                   */
#define X86_CR4_SMEP       0x00100000 /* SMEP                           */
#define X86_CR4_SMAP       0x00200000 /* SMAP                           */

/*
 * Model Specific Registers (MSR)
 */
#define MSR_APIC_BASE 0x0000001B

#define MSR_EFER   0xc0000080      /* Extended Feature Enable Register */
#define EFER_SCE   (_U64(1) << 0)  /* SYSCALL Enable */
#define EFER_LME   (_U64(1) << 8)  /* Long Mode Enable */
#define EFER_LMA   (_U64(1) << 10) /* Long Mode Active */
#define EFER_NXE   (_U64(1) << 11) /* No Execute Enable */
#define EFER_SVME  (_U64(1) << 12) /* Secure Virtual Machine Enable */
#define EFER_LMSLE (_U64(1) << 13) /* Long Mode Segment Limit Enable */
#define EFER_FFXSR (_U64(1) << 14) /* Fast FXSAVE/FXRSTOR */
#define EFER_TCE   (_U64(1) << 15) /* Translation Cache Extension */

#define MSR_STAR  0xc0000081
#define MSR_LSTAR 0xc0000082
#define MSR_CSTAR 0xc0000083
#define MSR_FMASK 0xc0000084

#define MSR_FS_BASE        0xc0000100
#define MSR_GS_BASE        0xc0000101
#define MSR_SHADOW_GS_BASE 0xc0000102

#define MSR_TSC_AUX 0xc0000103

/*
 * Exception mnemonics.
 */
#define X86_EX_DE  0  /* Divide Error. */
#define X86_EX_DB  1  /* Debug Exception. */
#define X86_EX_NMI 2  /* NMI. */
#define X86_EX_BP  3  /* Breakpoint. */
#define X86_EX_OF  4  /* Overflow. */
#define X86_EX_BR  5  /* BOUND Range. */
#define X86_EX_UD  6  /* Invalid Opcode. */
#define X86_EX_NM  7  /* Device Not Available. */
#define X86_EX_DF  8  /* Double Fault. */
#define X86_EX_CS  9  /* Coprocessor Segment Overrun. */
#define X86_EX_TS  10 /* Invalid TSS. */
#define X86_EX_NP  11 /* Segment Not Present. */
#define X86_EX_SS  12 /* Stack-Segment Fault. */
#define X86_EX_GP  13 /* General Porection Fault. */
#define X86_EX_PF  14 /* Page Fault. */
#define X86_EX_SPV 15 /* PIC Spurious Interrupt Vector. */
#define X86_EX_MF  16 /* Maths fault (x87 FPU). */
#define X86_EX_AC  17 /* Alignment Check. */
#define X86_EX_MC  18 /* Machine Check. */
#define X86_EX_XM  19 /* SIMD Exception. */
#define X86_EX_VE  20 /* Virtualisation Exception. */
#define X86_EX_SE  30 /* Security Exception. */

#define X86_EX_BIT(exception) (1 << (exception))

#define X86_EX_FAULT_BITMASK                                                             \
    (X86_EX_BIT(X86_EX_DE) | X86_EX_BIT(X86_EX_BR) | X86_EX_BIT(X86_EX_UD) |             \
     X86_EX_BIT(X86_EX_NM) | X86_EX_BIT(X86_EX_CS) | X86_EX_BIT(X86_EX_TS) |             \
     X86_EX_BIT(X86_EX_NP) | X86_EX_BIT(X86_EX_SS) | X86_EX_BIT(X86_EX_GP) |             \
     X86_EX_BIT(X86_EX_PF) | X86_EX_BIT(X86_EX_MF) | X86_EX_BIT(X86_EX_AC) |             \
     X86_EX_BIT(X86_EX_XM) | X86_EX_BIT(X86_EX_VE))

#define X86_EX_TRAP_BITMASK                                                              \
    (X86_EX_BIT(X86_EX_DB) | X86_EX_BIT(X86_EX_BP) | X86_EX_BIT(X86_EX_OF))

#define X86_EX_INTR_BITMASK (X86_EX_BIT(X86_EX_NMI) | X86_EX_BIT(X86_EX_SPV))

#define X86_EX_ABORT_BITMASK (X86_EX_BIT(X86_EX_DF) | X86_EX_BIT(X86_EX_MC))

#define X86_EX_HAS_ERROR_CODE                                                            \
    (X86_EX_BIT(X86_EX_DF) | X86_EX_BIT(X86_EX_TS) | X86_EX_BIT(X86_EX_NP) |             \
     X86_EX_BIT(X86_EX_SS) | X86_EX_BIT(X86_EX_GP) | X86_EX_BIT(X86_EX_PF) |             \
     X86_EX_BIT(X86_EX_AC) | X86_EX_BIT(X86_EX_SE))

#define X86_EX_PFEC_PRESENT 0x01
#define X86_EX_PFEC_WRITE   0x02
#define X86_EX_PFEC_USER    0x04
#define X86_EX_PFEC_RSVD    0x08
#define X86_EX_PFEC_FETCH   0x10

#define X86_EX_SEL_TLB_GDT  0x00
#define X86_EX_SEL_TLB_IDT  0x01
#define X86_EX_SEL_TLB_LDT  0x10
#define X86_EX_SEL_TLB_IDT2 0x11

#ifndef __ASSEMBLY__
union x86_ex_error_code {
    uint32_t error_code;
    struct __packed {
        unsigned int E : 1, TLB : 2, index : 13, rsvd_sel : 16;
    };
    struct __packed {
        unsigned int P : 1, W : 1, U : 1, R : 1, I : 1, rsvd_pfec : 27;
    };
};
typedef union x86_ex_error_code x86_ex_error_code_t;

/*
 * Exception handlers
 */
extern void entry_DE(void);
extern void entry_DB(void);
extern void entry_NMI(void);
extern void entry_BP(void);
extern void entry_OF(void);
extern void entry_BR(void);
extern void entry_UD(void);
extern void entry_NM(void);
extern void entry_DF(void);
extern void entry_CS(void);
extern void entry_TS(void);
extern void entry_NP(void);
extern void entry_SS(void);
extern void entry_GP(void);
extern void entry_PF(void);
extern void entry_MF(void);
extern void entry_AC(void);
extern void entry_MC(void);
extern void entry_XM(void);
extern void entry_VE(void);
extern void entry_SE(void);

extern void rmode_exception(void);

#if defined(__x86_64__)
typedef uint64_t x86_reg_t;
#elif defined(__i386__)
typedef uint32_t x86_reg_t;
#endif

struct cpu_regs {
    x86_reg_t r15;
    x86_reg_t r14;
    x86_reg_t r13;
    x86_reg_t r12;
    x86_reg_t r11;
    x86_reg_t r10;
    x86_reg_t r9;
    x86_reg_t r8;
    x86_reg_t _ASM_BP;
    x86_reg_t _ASM_DI;
    x86_reg_t _ASM_SI;
    x86_reg_t _ASM_DX;
    x86_reg_t _ASM_CX;
    x86_reg_t _ASM_BX;
    x86_reg_t _ASM_AX;

    x86_ex_error_code_t error_code;
    /* Populated by exception entry */
    uint32_t vector;

    /* Hardware exception */
    x86_reg_t _ASM_IP;
    uint16_t cs, _pad_cs[3];
    x86_reg_t _ASM_FLAGS;
    x86_reg_t _ASM_SP;
    uint16_t ss, _pad_ss[3];
};
typedef struct cpu_regs cpu_regs_t;

static inline bool has_error_code(uint32_t vector) {
    return !!((1U << vector) & X86_EX_HAS_ERROR_CODE);
}

#endif /* __ASSEMBLY__ */

#endif /* KTF_PROCESSOR_H */
