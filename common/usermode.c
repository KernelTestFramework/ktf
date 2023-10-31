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
#include <errno.h>
#include <lib.h>
#include <mm/vmm.h>
#include <pagetable.h>
#include <percpu.h>
#include <processor.h>
#include <traps.h>
#include <usermode.h>

extern char exception_handlers[], end_exception_handlers[];
extern char interrupt_handlers[], end_interrupt_handlers[];
extern char usermode_helpers[], end_usermode_helpers[];

long syscall_handler(long syscall_nr, long arg1, long arg2, long arg3, long arg4,
                     long arg5) {
    switch (syscall_nr) {
    case SYSCALL_EXIT:
        /* SYSCALL_EXIT is handled by asm routine syscall_exit() */
        UNREACHABLE();

    case SYSCALL_PRINTF:
        vprintk(_ptr(arg1), _ptr(arg2));
        return 0;

    case SYSCALL_MMAP: {
        void *va = _ptr(arg1);
        unsigned int order = _u(arg2);
        frame_t *frame;

        frame = get_free_frames(order);
        if (!frame)
            return -ENOMEM;

        if (!va)
            va = mfn_to_virt_user(frame->mfn);
        va = vmap_user(va, frame->mfn, order, L4_PROT_USER, L3_PROT_USER, L2_PROT_USER,
                       L1_PROT_USER);
        return _ul(va);
    }

    case SYSCALL_MUNMAP: {
        void *va = _ptr(arg1);
        unsigned int order = _u(arg2);

        vunmap_user(va, order);
        return 0;
    }

    default:
        printk("Unknown syscall: %lu\n", syscall_nr);
        return -1;
    }
}

static void init_syscall(void) {
    msr_star_t star;

    star.eip = _u(_ul(&syscall_handler_entry));

    /* see segment.h for details regarding these values */
    star.kern_cs = __KERN_CS;
    star.user_cs = __USER_CS32;

    wrmsr(MSR_STAR, star.reg);
    wrmsr(MSR_LSTAR, _ul(&syscall_handler_entry));
    /* FIXME: Add compat support */
    wrmsr(MSR_CSTAR, _ul(NULL));

    wrmsr(MSR_FMASK, USERMODE_FLAGS_MASK);
    wrmsr(MSR_EFER, rdmsr(MSR_EFER) | EFER_SCE);
}

static void init_sysenter(percpu_t *percpu) {
    wrmsr(MSR_SYSENTER_CS, _ul(__KERN_CS));
    wrmsr(MSR_SYSENTER_ESP, _ul(percpu->tss.rsp0));
    wrmsr(MSR_SYSENTER_EIP, _ul(&sysenter_handler_entry));
}

void init_usermode(percpu_t *percpu) {
    vmap_user_4k(&cr3, virt_to_mfn(&cr3), L1_PROT);
    vmap_user_4k(&user_cr3, virt_to_mfn(&user_cr3), L1_PROT);

    BUG_ON(end_exception_handlers - exception_handlers > (long) PAGE_SIZE);
    vmap_user_4k(exception_handlers, virt_to_mfn(exception_handlers), L1_PROT);

    BUG_ON(end_interrupt_handlers - interrupt_handlers > (long) PAGE_SIZE);
    vmap_user_4k(interrupt_handlers, virt_to_mfn(interrupt_handlers), L1_PROT);

    BUG_ON(end_usermode_helpers - usermode_helpers > (long) PAGE_SIZE);
    vmap_user_4k(usermode_helpers, virt_to_mfn(usermode_helpers), L1_PROT);

    init_syscall();
    init_sysenter(percpu);
    set_intr_gate(&percpu->idt[SYSCALL_INT], __KERN_CS, _ul(int80_handler_entry),
                  GATE_DPL3, GATE_PRESENT, 0);
}

static inline long __user_text _int80(long syscall_nr, long arg1, long arg2, long arg3,
                                      long arg4, long arg5) {
    register long return_code asm(STR(_ASM_AX));
    register long _arg4 asm("r8") = arg4;
    register long _arg5 asm("r9") = arg5;

    /* clang-format off */
    asm volatile(
        "int $" STR(SYSCALL_INT) "\n"
        : "=a"(return_code)
        : "0"(syscall_nr), "S"(arg1), "d"(arg2), "D" (arg3), "r"(_arg4), "r"(_arg5)
    );
    /* clang-format on */

    return return_code;
}

static inline long __user_text _syscall(long syscall_nr, long arg1, long arg2, long arg3,
                                        long arg4, long arg5) {
    register long return_code asm(STR(_ASM_AX));
    register long _arg4 asm("r8") = arg4;
    register long _arg5 asm("r9") = arg5;

    /* clang-format off */
    asm volatile(
        "syscall\n"
        : "=a"(return_code)
        : "0"(syscall_nr), "S"(arg1), "d"(arg2), "D" (arg3), "r"(_arg4), "r"(_arg5)
        : STR(_ASM_CX), "r11"
    );
    /* clang-format on */

    return return_code;
}

static inline long __user_text _sysenter(long syscall_nr, long arg1, long arg2, long arg3,
                                         long arg4, long arg5) {
    register long return_code asm(STR(_ASM_AX));
    register long _arg4 asm("r8") = arg4;
    register long _arg5 asm("r9") = arg5;
    register long _arg3 asm("r10") = arg3;

    /* clang-format off */
    asm volatile (
        "mov %%" STR(_ASM_SP) ", %%" STR(_ASM_CX) "\n"
        "lea 1f(%%" STR(_ASM_IP) "), %%" STR(_ASM_DX) "\n"
        "sysenter\n"
        "1: "
        : "=a"(return_code)
        : "0"(syscall_nr), "S"(arg1), "D" (arg2), "r" (_arg3), "r"(_arg4), "r"(_arg5)
        : STR(_ASM_CX), STR(_ASM_DX)
    );
    /* clang-format on */

    return return_code;
}

static __user_data syscall_mode_t sc_mode = SYSCALL_MODE_SYSCALL;

bool __user_text syscall_mode(syscall_mode_t mode) {
    if (mode > SYSCALL_MODE_INT80) {
        return false;
    }
    sc_mode = mode;

    return true;
}

static long __user_text syscall(long syscall_nr, long arg1, long arg2, long arg3,
                                long arg4, long arg5) {
    switch (sc_mode) {
    case SYSCALL_MODE_SYSCALL:
        return _syscall(syscall_nr, arg1, arg2, arg3, arg4, arg5);
    case SYSCALL_MODE_SYSENTER:
        return _sysenter(syscall_nr, arg1, arg2, arg3, arg4, arg5);
    case SYSCALL_MODE_INT80:
        return _int80(syscall_nr, arg1, arg2, arg3, arg4, arg5);
    default:
        UNREACHABLE();
    }
}

#define syscall0(nr)                     syscall((nr), 0, 0, 0, 0, 0)
#define syscall1(nr, a1)                 syscall((nr), (a1), 0, 0, 0, 0)
#define syscall2(nr, a1, a2)             syscall((nr), (a1), (a2), 0, 0, 0)
#define syscall3(nr, a1, a2, a3)         syscall((nr), (a1), (a2), (a3), 0, 0)
#define syscall4(nr, a1, a2, a3, a4)     syscall((nr), (a1), (a2), (a3), (a4), 0)
#define syscall5(nr, a1, a2, a3, a4, a5) syscall((nr), (a1), (a2), (a3), (a4), (a5))

static inline void __user_text sys_exit(unsigned long exit_code) {
    syscall1(SYSCALL_EXIT, exit_code);
}

static inline long __user_text sys_printf(const char *fmt, va_list args) {
    return syscall2(SYSCALL_PRINTF, _ul(fmt), _ul(args));
}

static inline long __user_text sys_mmap(void *va, unsigned long order) {
    return syscall2(SYSCALL_MMAP, _ul(va), order);
}

static inline long __user_text sys_munmap(void *va, unsigned long order) {
    return syscall2(SYSCALL_MUNMAP, _ul(va), order);
}

void __user_text exit(unsigned long exit_code) {
    sys_exit(exit_code);
}

void __user_text printf(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    sys_printf(fmt, args);
    va_end(args);
}

void *__user_text mmap(void *va, unsigned long order) {
    return _ptr(sys_mmap(va, order));
}

void __user_text munmap(void *va, unsigned long order) {
    sys_munmap(va, order);
}
