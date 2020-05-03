#ifndef KTF_PROCESSOR_H
#define KTF_PROCESSOR_H

#include <compiler.h>

/*
 * EFLAGS bits.
 */
#define X86_EFLAGS_CF      0x00000001 /* Carry Flag                */
#define X86_EFLAGS_MBS     0x00000002 /* Resvd bit                 */
#define X86_EFLAGS_PF      0x00000004 /* Parity Flag               */
#define X86_EFLAGS_AF      0x00000010 /* Auxillary carry Flag      */
#define X86_EFLAGS_ZF      0x00000040 /* Zero Flag                 */
#define X86_EFLAGS_SF      0x00000080 /* Sign Flag                 */
#define X86_EFLAGS_TF      0x00000100 /* Trap Flag                 */
#define X86_EFLAGS_IF      0x00000200 /* Interrupt Flag            */
#define X86_EFLAGS_DF      0x00000400 /* Direction Flag            */
#define X86_EFLAGS_OF      0x00000800 /* Overflow Flag             */
#define X86_EFLAGS_IOPL    0x00003000 /* IOPL mask                 */
#define X86_EFLAGS_NT      0x00004000 /* Nested Task               */
#define X86_EFLAGS_RF      0x00010000 /* Resume Flag               */
#define X86_EFLAGS_VM      0x00020000 /* Virtual Mode              */
#define X86_EFLAGS_AC      0x00040000 /* Alignment Check           */
#define X86_EFLAGS_VIF     0x00080000 /* Virtual Interrupt Flag    */
#define X86_EFLAGS_VIP     0x00100000 /* Virtual Interrupt Pending */
#define X86_EFLAGS_ID      0x00200000 /* CPUID detection flag      */

/*
 * CPU flags in CR0.
 */
#define X86_CR0_PE         0x00000001 /* Enable Protected Mode    (RW) */
#define X86_CR0_MP         0x00000002 /* Monitor Coprocessor      (RW) */
#define X86_CR0_EM         0x00000004 /* Require FPU Emulation    (RO) */
#define X86_CR0_TS         0x00000008 /* Task Switched            (RW) */
#define X86_CR0_ET         0x00000010 /* Extension type           (RO) */
#define X86_CR0_NE         0x00000020 /* Numeric Error Reporting  (RW) */
#define X86_CR0_WP         0x00010000 /* Supervisor Write Protect (RW) */
#define X86_CR0_AM         0x00040000 /* Alignment Checking       (RW) */
#define X86_CR0_NW         0x20000000 /* Not Write-Through        (RW) */
#define X86_CR0_CD         0x40000000 /* Cache Disable            (RW) */
#define X86_CR0_PG         0x80000000 /* Paging                   (RW) */

/*
 * CPU features in CR4.
 */
#define X86_CR4_VME        0x00000001  /* VM86 extensions                */
#define X86_CR4_PVI        0x00000002  /* Virtual interrupts flag        */
#define X86_CR4_TSD        0x00000004  /* Disable time stamp at ipl 3    */
#define X86_CR4_DE         0x00000008  /* Debugging extensions           */
#define X86_CR4_PSE        0x00000010  /* Page size extensions           */
#define X86_CR4_PAE        0x00000020  /* Physical address extensions    */
#define X86_CR4_MCE        0x00000040  /* Machine check                  */
#define X86_CR4_PGE        0x00000080  /* Global pages                   */
#define X86_CR4_PCE        0x00000100  /* Performance counters at ipl 3  */
#define X86_CR4_OSFXSR     0x00000200  /* Fast FPU save and restore      */
#define X86_CR4_OSXMMEXCPT 0x00000400  /* Unmasked SSE exceptions        */
#define X86_CR4_UMIP       0x00000800  /* UMIP                           */
#define X86_CR4_VMXE       0x00002000  /* VMX                            */
#define X86_CR4_SMXE       0x00004000  /* SMX                            */
#define X86_CR4_FSGSBASE   0x00010000  /* {rd,wr}{fs,gs}base             */
#define X86_CR4_PCIDE      0x00020000  /* PCID                           */
#define X86_CR4_OSXSAVE    0x00040000  /* XSAVE/XRSTOR                   */
#define X86_CR4_SMEP       0x00100000  /* SMEP                           */
#define X86_CR4_SMAP       0x00200000  /* SMAP                           */

/*
 * Model Specific Registers (MSR)
 */
#define MSR_EFER           0xc0000080      /* Extended Feature Enable Register */
#define EFER_SCE           (_U64(1) <<  0) /* SYSCALL Enable */
#define EFER_LME           (_U64(1) <<  8) /* Long Mode Enable */
#define EFER_LMA           (_U64(1) << 10) /* Long Mode Active */
#define EFER_NXE           (_U64(1) << 11) /* No Execute Enable */
#define EFER_SVME          (_U64(1) << 12) /* Secure Virtual Machine Enable */
#define EFER_LMSLE         (_U64(1) << 13) /* Long Mode Segment Limit Enable */
#define EFER_FFXSR         (_U64(1) << 14) /* Fast FXSAVE/FXRSTOR */
#define EFER_TCE           (_U64(1) << 15) /* Translation Cache Extension */

#define MSR_STAR           0xc0000081
#define MSR_LSTAR          0xc0000082
#define MSR_CSTAR          0xc0000083
#define MSR_FMASK          0xc0000084

#define MSR_FS_BASE        0xc0000100
#define MSR_GS_BASE        0xc0000101
#define MSR_SHADOW_GS_BASE 0xc0000102

#define MSR_TSC_AUX        0xc0000103

#endif /* KTF_PROCESSOR_H */
