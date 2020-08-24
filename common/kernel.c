/*
 * Copyright © 2020 Amazon.com, Inc. or its affiliates.
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
#include <multiboot.h>
#include <percpu.h>
#include <sched.h>
#include <setup.h>

extern void _long_to_real(void);

extern int usermode_call_asm(user_func_t fn, void *fn_arg, unsigned long ret2kern_sp,
                             unsigned long user_stack);

void ret2kern_handler(void) {
    asm volatile("mov %%gs:(%0), %%" STR(_ASM_SP)::"r"(offsetof(percpu_t, ret2kern_sp)));
}

int usermode_call(user_func_t fn, void *fn_arg) {
    return usermode_call_asm(fn, fn_arg, offsetof(percpu_t, ret2kern_sp),
                             offsetof(percpu_t, user_stack));
}

void kernel_main(void) {
    printk("\nKTF - Kernel Test Framework!\n\n");

    if (kernel_cmdline)
        printk("Command line: %s\n", kernel_cmdline);

    zap_boot_mappings();
    display_memory_map();
    display_multiboot_mmap();

    if (opt_debug) {
        _long_to_real();
        printk("\n After long_to_real\n");
    }

    test_main();

    printk("All tasks done.\n");

    while (1)
        halt();
}
