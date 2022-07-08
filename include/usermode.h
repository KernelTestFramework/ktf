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
#ifndef KTF_USERMODE_H
#define KTF_USERMODE_H

#define SYSCALL_EXIT   0
#define SYSCALL_PRINTF 1
#define SYSCALL_MMAP   2
#define SYSCALL_MUNMAP 3

#ifndef __ASSEMBLY__
#include <percpu.h>
#include <sched.h>

/* Static declarations */

static inline bool from_usermode(uint16_t cs) {
    return (cs & 0x3) != 0;
}

static inline void goto_syscall_exit(long exit_code) {
    swapgs();
    asm volatile("jmp syscall_handler" ::"A"(SYSCALL_EXIT), "D"(exit_code));
}

/* External declarations */

extern unsigned long enter_usermode(task_func_t fn, void *fn_arg,
                                    unsigned long usermode_private, void *user_stack);
extern void __naked syscall_handler(void);

extern void init_usermode(percpu_t *percpu);

extern void __user_text exit(unsigned long exit_code);
extern void __user_text printf(const char *fmt, unsigned long arg1, unsigned long arg2,
                               unsigned long arg3);
extern void *__user_text mmap(void *va, unsigned long order);
extern void __user_text munmap(void *va, unsigned long order);

#endif /* __ASSEMBLY__ */

#endif /* KTF_USERMODE_H */
