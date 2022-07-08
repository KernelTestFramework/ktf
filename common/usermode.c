/*
 * Copyright © 2022 Open Source Security, Inc.
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
#include <lib.h>
#include <pagetable.h>
#include <processor.h>

static inline void syscall_save(void) {
    /* clang-format off */
    asm volatile(
        "push %%" STR(_ASM_CX) "\n"
        "push %%r11"
        ::: "memory"
    );
    /* clang-format on */
}

static inline void syscall_restore(void) {
    /* clang-format off */
    asm volatile(
        "pop %%r11\n"
        "pop %%" STR(_ASM_CX)
        ::: "memory"
    );
    /* clang-format on */
}

static inline long syscall_return(long return_code) {
    register long ret asm(STR(_ASM_AX)) = return_code;
    return ret;
}

static inline void switch_address_space(const cr3_t *cr3) {
    /* clang-format off */
    asm volatile(
        "push %%" STR(_ASM_AX) "\n"
        "mov %[cr3], %%" STR(_ASM_AX) "\n"
        "mov %%" STR(_ASM_AX) ", %%cr3\n"
        "pop %%" STR(_ASM_AX) "\n"
        :: [cr3] "m" (*cr3)
        : STR(_ASM_AX)
    );
    /* clang-format on */
}

void __naked syscall_handler(void) {
    sysret();
}

static void init_syscall(void) {
    msr_star_t star;

    star.eip = _u(_ul(&syscall_handler));
    star.kern_cs = __KERN_CS64;
    star.user_cs = __USER_CS64;

    wrmsr(MSR_STAR, star.reg);
    wrmsr(MSR_LSTAR, _ul(&syscall_handler));
    /* FIXME: Add compat support */
    wrmsr(MSR_CSTAR, _ul(NULL));

    wrmsr(MSR_FMASK, X86_EFLAGS_CF | X86_EFLAGS_PF | X86_EFLAGS_AF | X86_EFLAGS_ZF |
                         X86_EFLAGS_SF | X86_EFLAGS_TF | X86_EFLAGS_IF | X86_EFLAGS_DF |
                         X86_EFLAGS_OF | X86_EFLAGS_ID | X86_EFLAGS_NT | X86_EFLAGS_RF |
                         X86_EFLAGS_AC | X86_EFLAGS_IOPL);

    wrmsr(MSR_EFER, rdmsr(MSR_EFER) | EFER_SCE);
}

void init_usermode(percpu_t *percpu) {
    vmap_user_4k(&cr3, virt_to_mfn(&cr3), L1_PROT);
    init_syscall();
}
