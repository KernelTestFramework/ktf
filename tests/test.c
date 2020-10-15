/*
 * Copyright (c) 2020 Amazon.com, Inc. or its affiliates.
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
#include <sched.h>

#ifdef KTF_UNIT_TEST
#include <cmdline.h>
#include <string.h>

extern char *kernel_cmdline;

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
#endif

static int __user_text func(void *arg) { return 0; }

void test_main(void) {
    printk("\nTest:\n");

    usermode_call(func, NULL);

#ifdef KTF_UNIT_TEST
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
        printk("Integer parameter opt_ulong != 42: %d\n", opt_ulong);
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
#endif

    wait_for_all_tasks();

    printk("Test done\n");
}
