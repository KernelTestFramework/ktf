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
#include <string.h>
#include <symbols.h>
#include <test.h>

static const char opt_test_delims[] = ",";
#include <cmdline.h>

enum get_next_test_result {
    TESTS_DONE,
    TESTS_FOUND,
    TESTS_ERROR,
};
typedef enum get_next_test_result get_next_test_result_t;

static char opt_tests[MAX_OPT_TESTS_LEN];
string_cmd("tests", opt_tests);

static get_next_test_result_t get_next_test(test_fn **out_test_fn, char **out_name) {
    static char *opt = opt_tests;

    *out_name = strtok(opt, opt_test_delims);

    opt = NULL;
    if (*out_name) {
        *out_test_fn = symbol_address(*out_name);
        if (!*out_test_fn) {
            printk("Symbol for test %s not found\n", *out_name);
            return TESTS_ERROR;
        }
        return TESTS_FOUND;
    }
    return TESTS_DONE;
}

void test_main(void *unused) {
    char *name;
    test_fn *fn = NULL;
    unsigned n = 0;

    printk("\nRunning tests\n");

    while (get_next_test(&fn, &name) == TESTS_FOUND) {
        int rc;

        printk("Running test: %s\n", name);
        rc = fn(NULL);
        printk("Test %s returned: 0x%x\n", name, rc);
        n++;
    }

    printk("Tests completed: %u\n", n);
}
