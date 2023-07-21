/*
 * Copyright (c) 2021 Amazon.com, Inc. or its affiliates.
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
#include <cmdline.h>
#include <console.h>
#include <cpuid.h>
#include <ktf.h>
#include <real_mode.h>
#include <sched.h>
#include <string.h>
#include <symbols.h>
#include <test.h>
#include <usermode.h>

#include <smp/smp.h>

static char opt_string[4];
string_cmd("string", opt_string);

static char opt_badstring[5];
string_cmd("badstring", opt_badstring);

static unsigned long opt_ulong;
ulong_cmd("integer", opt_ulong);

static bool opt_bool = 0;
bool_cmd("boolean", opt_bool);

static bool opt_booltwo = 0;
bool_cmd("booleantwo", opt_booltwo);

static char memmove_string[4];
static char range_string[] = "123456";
static char *src, *dst;

static void cpu_freq_expect(const char *cpu_str, uint64_t expectation) {
    uint64_t result = get_cpu_freq(cpu_str);
    if (result != expectation) {
        printk("Could not parse cpu frequency from string '%s'\n", cpu_str);
        printk("Expectation vs. result: %llu vs. %llu\n", expectation, result);
        BUG();
    }

    printk("Got CPU string '%s' and frequency '%llu'\n", cpu_str, result);
    return;
}

static unsigned long test_kernel_task_func(void *arg) {
    printk("CPU[%u]: Executing %s\n", smp_processor_id(), __func__);
    return _ul(arg);
}

static unsigned long __user_text test_user_task_func1(void *arg) {
    static char *fmt_printf __user_data = "printf: %u %x %d\n";

    printf(fmt_printf, 1234, 0x41414141, 9);
    printf(fmt_printf, 1235, 0x42424242, -9);

    exit(9);
    return 0;
}

static unsigned long __user_text test_user_task_func2(void *arg) {
    static char *fmt_mmap __user_data = "mmap: %lx\n";
    void *va;

    va = mmap(_ptr(0xfff80000), PAGE_ORDER_4K);
    printf(fmt_mmap, _ul(va), 0, 0);

    memset(va, 0xcc, 0x1000);
    ((void (*)(void)) va)();

    return 0;
}

int unit_tests(void *_unused) {
    printk("\nLet the UNITTESTs begin\n");
    printk("Commandline parsing: %s\n", kernel_cmdline);

    if (strcmp(opt_string, "foo")) {
        printk("String parameter opt_string != foo: %s\n", opt_string);
        BUG();
    }
    else {
        printk("String parameter parsing works!\n");
    }

    if (strcmp(opt_badstring, "tool")) {
        printk("String parameter opt_badstring != tool: %s\n", opt_badstring);
        BUG();
    }
    else {
        printk("String parameter parsing works!\n");
    }

    if (opt_ulong != 42) {
        printk("Integer parameter opt_ulong != 42: %lu\n", opt_ulong);
        BUG();
    }
    else {
        printk("Integer parameter parsing works!\n");
    }

    if (!opt_bool || !opt_booltwo) {
        printk("Boolean parameter opt_bool != true: %d\n", opt_bool);
        printk("Boolean parameter opt_booltwo != true: %d\n", opt_booltwo);
        BUG();
    }
    else {
        printk("Boolean parameter parsing works!\n");
    }

    printk("\nMemmove testing:\n");
    (void) memmove(memmove_string, opt_string, sizeof(opt_string));
    if (!strcmp(memmove_string, opt_string)) {
        printk("Moving around memory works!\n");
    }
    else {
        printk("Memmove'ing did not work: %s (%p) != %s (%p)\n", memmove_string,
               memmove_string, opt_string, opt_string);
    }

    src = (char *) range_string;
    dst = (char *) range_string + 2;
    (void) memmove(dst, src, 4);
    if (!strcmp(range_string, "121234")) {
        printk("Moving around memory with overlaping ranges works!\n");
    }
    else {
        printk("Overlaping memmove'ing did not work: %s != %s\n", range_string, "121234");
    }

    cpu_freq_expect("Intel(R) Xeon(R) CPU E3-1270 V2 @ 3.50GHz", 3500000000);
    cpu_freq_expect("Intel(R) Celeron(R) CPU J1900 @ 1.99GHz", 1990000000);
    cpu_freq_expect("AMD G-T48L Processor", 0);
    cpu_freq_expect("Intel(R) Atom(TM) CPU E3815 @ 1.46GHz", 1460000000);
    cpu_freq_expect("Intel(R) Atom(TM) CPU E620 @ 600MHz", 600000000);
    cpu_freq_expect("VIA C7 Processor 1000MHz", 1000000000);
    cpu_freq_expect("Intel(R) Core(TM) i7 CPU 950 @ 3.07GHz", 3070000000);
    cpu_freq_expect("AMD Ryzen Threadripper 1950X 16-Core Processor", 0);
    cpu_freq_expect("Prototyp Amazing Foo One @ 1GHz", 1000000000);
    cpu_freq_expect("Prototyp Amazing Foo Two @ 1.00GHz", 1000000000);

    task_t *task1, *task2, *task_user1, *task_user2;

    task1 = new_kernel_task("test1", test_kernel_task_func, _ptr(98));
    task2 = new_kernel_task("test2", test_kernel_task_func, _ptr(-99));
    task_user1 = new_user_task("test1 user", test_user_task_func1, NULL);
    task_user2 = new_user_task("test2 user", test_user_task_func2, NULL);

    set_task_repeat(task1, 10);
    schedule_task(task1, get_bsp_cpu());
    schedule_task(task2, get_cpu(1));
    schedule_task(task_user1, get_bsp_cpu());
    schedule_task(task_user2, get_cpu(1));

#ifdef UNITTEST_LONGMODE
    printk("Long mode to real mode transition:\n");
    long_to_real();
#endif

    return 0;
}
