#include <asm-macros.h>
#include <processor.h>
#include <segment.h>
#include <page.h>
#include <traps.h>

.macro exception_handler sym vec has_error_code
ENTRY(entry_\sym)
    .if \has_error_code == 0
        push $0
    .endif

    movl $\vec, 0x4(%_ASM_SP)
    jmp handle_exception
END_FUNC(entry_\sym)
.endm

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

ENTRY(handle_exception)
    cld
    SAVE_REGS

    mov %_ASM_SP, %_ASM_DI
    call do_exception

    RESTORE_REGS

#if defined(__x86_64__)
    add $8, %_ASM_SP
    iretq
#else
    add $4, %_ASM_SP
    iret
#endif
END_FUNC(handle_exception)

ENTRY(usermode_call_asm)
    /* FIXME: Add 32-bit support */

    mov %_ASM_SP, %gs:(%rdx)

    /* SS + SP */
    push $__USER_DS
    push %gs:(%rcx)

    /* EFLAGS */
#if defined(__x86_64__)
    pushfq
#else
    pushf
#endif

    orl $X86_EFLAGS_IOPL, (%_ASM_SP)

    /* CS + IP */
    pushq $__USER_CS
    push $usermode_stub

#if defined(__x86_64__)
    iretq
#else
    iret
#endif
END_FUNC(usermode_call_asm)

SECTION(.text.user, "ax", 16)
ENTRY(usermode_stub)
    /* DI: User function to be called
     * SI: Parameters
     * ...
     */
    xchg %_ASM_DI, %_ASM_SI
    call *%_ASM_SI

    int $X86_RET2KERN_INT
END_FUNC(usermode_stub)
