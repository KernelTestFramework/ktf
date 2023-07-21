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
#include <percpu.h>
#include <processor.h>
#include <usermode.h>

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
    /* clang-format off */
    asm volatile(
            "mov %[ret], %%" STR(_ASM_AX) "\n"
            ::[ret] "r"(return_code)
    );
    /* clang-format on */
    return return_code;
}

static inline void stack_switch(void) {
    /* clang-format off */
    asm volatile(
        "xchg %%gs:%[private], %%" STR(_ASM_SP) "\n"
        ::[private] "m"(ACCESS_ONCE(PERCPU_VAR(usermode_private)))
    );
    /* clang-format on */
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

static inline void _sys_exit(void) {
    /* clang-format off */
    asm volatile (
        "mov %%" STR(_ASM_DI)", %%gs:%[private]\n"
        POPF()
        RESTORE_ALL_REGS()
        "mov %%gs:%[private], %%" STR(_ASM_AX) "\n"
        "ret\n"
        :: [ private ] "m"(ACCESS_ONCE(PERCPU_VAR(usermode_private)))
        : STR(_ASM_AX)
    );
    /* clang-format on */
}

void __naked syscall_handler(void) {
    register unsigned long syscall_nr asm(STR(_ASM_AX));
    register unsigned long param1 asm(STR(_ASM_DI));
    (void) param1;
    register unsigned long param2 asm(STR(_ASM_SI));
    (void) param2;
    register unsigned long param3 asm(STR(_ASM_BX));
    (void) param3;
    register unsigned long param4 asm(STR(_ASM_DX));
    (void) param4;

    SAVE_CLOBBERED_REGS();
    switch_address_space(&cr3);
    swapgs();
    stack_switch();
    syscall_save();

    switch (syscall_nr) {
    case SYSCALL_EXIT:
        syscall_restore();
        _sys_exit();
        UNREACHABLE();

    case SYSCALL_PRINTF:
        printk(_ptr(param1), param2, param3, param4);
        syscall_return(0);
        break;

    case SYSCALL_MMAP: {
        void *va = _ptr(param1);
        unsigned int order = _u(param2);
        frame_t *frame;

        frame = get_free_frames(order);
        if (!va)
            va = mfn_to_virt_user(frame->mfn);

        va = vmap_user(va, frame->mfn, order, L4_PROT_USER, L3_PROT_USER, L2_PROT_USER,
                       L1_PROT_USER);
        syscall_return(_ul(va));
    } break;

    case SYSCALL_MUNMAP: {
        void *va = _ptr(param1);
        unsigned int order = _u(param2);

        vunmap_user(va, order);
    } break;

    default:
        printk("Unknown syscall: %lu\n", syscall_nr);
        syscall_return(-1L);
        break;
    }

    syscall_restore();
    stack_switch();
    swapgs();
    switch_address_space(&user_cr3);

    RESTORE_CLOBBERED_REGS();

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
    vmap_user_4k(&enter_usermode, virt_to_mfn(&enter_usermode), L1_PROT);
    vmap_user_4k(&syscall_handler, virt_to_mfn(&syscall_handler), L1_PROT);

    init_syscall();
}

static inline void __user_text sys_exit(unsigned long exit_code) {
    asm volatile("syscall" ::"A"(SYSCALL_EXIT), "D"(exit_code) : STR(_ASM_CX), "r11");
}

static inline long __user_text sys_printf(const char *fmt, unsigned long arg1,
                                          unsigned long arg2, unsigned long arg3) {
    register unsigned long rax asm(STR(_ASM_AX));

    /* clang-format off */
    asm volatile(
        "syscall"
        : "=A"(rax)
        : "0"(SYSCALL_PRINTF), "D"(fmt), "S"(arg1), "b"(arg2), "d"(arg3)
        : STR(_ASM_CX), "r11"
    );
    /* clang-format on */

    return rax;
}

static inline long __user_text sys_mmap(void *va, unsigned long order) {
    register unsigned long rax asm(STR(_ASM_AX));

    /* clang-format off */
    asm volatile(
        "syscall"
        : "=A"(rax)
        : "0"(SYSCALL_MMAP), "D"(va), "S"(order)
        : STR(_ASM_CX), "r11"
    );
    /* clang-format on */

    return rax;
}

static inline long __user_text sys_munmap(void *va, unsigned long order) {
    register unsigned long rax asm(STR(_ASM_AX));

    /* clang-format off */
    asm volatile(
        "syscall"
        ::"A"(SYSCALL_MUNMAP), "D"(va), "S"(order)
        : STR(_ASM_CX), "r11"
    );
    /* clang-format on */

    return rax;
}

void __user_text exit(unsigned long exit_code) {
    sys_exit(exit_code);
}

void __user_text printf(const char *fmt, unsigned long arg1, unsigned long arg2,
                        unsigned long arg3) {
    sys_printf(fmt, arg1, arg2, arg3);
}

void *__user_text mmap(void *va, unsigned long order) {
    return _ptr(sys_mmap(va, order));
}

void __user_text munmap(void *va, unsigned long order) {
    sys_munmap(va, order);
}
