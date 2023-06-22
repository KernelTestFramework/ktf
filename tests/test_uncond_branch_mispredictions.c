/*
 * Copyright (c) 2022 Open Source Security, Inc.
 * All Rights Reserved.
 *
 * Author: Pawel Wieczorkiewicz <wipawel@grsecurity.net>
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
#include <ktf.h>
#include <mm/pmm.h>
#include <string.h>
#include <toolkit/cache/lib.h>

/* Set it to 1 to enable BTB flushing */
#define WITH_BTB_FLUSH 0

/* Set it to 1 to use call as the direct unconditional branch */
#define TEST_CALL 0

#define ITERATIONS 10

#if WITH_BTB_FLUSH
#define LOOP_ITERATIONS (10 * 1000)
#else
#define LOOP_ITERATIONS (100 * 1000)
#endif

static unsigned baseline, counts[2];
static cache_channel_t *channel;

static cache_line_t *cl0;
static cache_line_t *cl1;

#define CHANNEL_ADDR      0x200000
#define CACHE_LINE_0_ADDR 0x2007c0
#define CACHE_LINE_1_ADDR 0x200f80

#if WITH_BTB_FLUSH
#define FLUSH_BTB()                                                                      \
    do {                                                                                 \
        asm volatile(".align 64\n    "                                                   \
                     ".rept 8192\n   "                                                   \
                     "    jmp 1f\n   "                                                   \
                     "    .rept 30\n "                                                   \
                     "        nop\n  "                                                   \
                     "    .endr\n    "                                                   \
                     "1:  jmp 2f\n   "                                                   \
                     "    .rept 29\n "                                                   \
                     "        nop\n  "                                                   \
                     "    .endr\n    "                                                   \
                     "2:  nop\n      "                                                   \
                     ".endr\n        ");                                                 \
    } while (0)
#else
#define FLUSH_BTB()
#endif

#if TEST_CALL
#define BRANCH call
#define CLEAN_STACK()                                                                    \
    do {                                                                                 \
        asm volatile("add $8, %rsp");                                                    \
    } while (0)
#else
#define BRANCH jmp
#define CLEAN_STACK()
#endif

void __aligned(PAGE_SIZE) test_uncond_backward_branch_cl0(unsigned iterations) {
    for (int i = 0; i < ITERATIONS; i++) {
        counts[0] = counts[1] = 0;

        for (unsigned j = 0; j < iterations; j++) {
            FLUSH_BTB();
            clflush(cl0);
            clflush(cl1);
            mfence();

            /* clang-format off */
            asm goto(
                "mov $%c[cl0], %%rsi\n"

                "jmp 1f\n"
                "2: jmp %l[end]\n"
                ".align 64\n"
                "lfence\n"
                "ud2\n"
                ".align 64\n"

                "1: " STR(BRANCH) " 2b\n"
                "mov (%%rsi), %%rax\n"
                :: [cl0] "i" (CACHE_LINE_0_ADDR)
                : "rsi", "rax", "memory"
                : end);
            /* clang-format on */
        end:
            CLEAN_STACK();
            counts[cache_channel_measure_bit(cl0, cl1, baseline)]++;
        }

        printk("%s," STR(CACHE_LINE_0_ADDR) ",%u,%u,%u,%u\n", __func__, counts[0],
               counts[1], iterations, baseline);
    }
}

void __aligned(PAGE_SIZE) test_uncond_backward_branch_cl1(unsigned iterations) {
    for (int i = 0; i < ITERATIONS; i++) {
        counts[0] = counts[1] = 0;

        for (unsigned j = 0; j < iterations; j++) {
            FLUSH_BTB();
            clflush(cl0);
            clflush(cl1);
            mfence();

            /* clang-format off */
            asm goto(
                "mov $%c[cl1], %%rsi\n"

                "jmp 1f\n"
                "2: jmp %l[end]\n"
                ".align 64\n"
                "lfence\n"
                "ud2\n"
                ".align 64\n"

                "1: " STR(BRANCH) " 2b\n"
                "mov (%%rsi), %%rax\n"
                :: [cl1] "i" (CACHE_LINE_1_ADDR)
                : "rsi", "rax", "memory"
                : end);
            /* clang-format on */
        end:
            CLEAN_STACK();
            counts[cache_channel_measure_bit(cl0, cl1, baseline)]++;
        }

        printk("%s," STR(CACHE_LINE_1_ADDR) ",%u,%u,%u,%u\n", __func__, counts[0],
               counts[1], iterations, baseline);
    }
}

void __aligned(PAGE_SIZE) test_uncond_forward_branch_cl0(unsigned iterations) {
    for (int i = 0; i < ITERATIONS; i++) {
        counts[0] = counts[1] = 0;

        for (unsigned j = 0; j < iterations; j++) {
            FLUSH_BTB();
            clflush(cl0);
            clflush(cl1);
            mfence();

            /* clang-format off */
            asm goto(
                "mov $%c[cl0], %%rsi\n"

                STR(BRANCH) " %l[end]\n"
                "mov (%%rsi), %%rax\n"
                :: [cl0] "i" (CACHE_LINE_0_ADDR)
                : "rsi", "rax", "memory"
                : end);
            /* clang-format on */
        end:
            CLEAN_STACK();
            counts[cache_channel_measure_bit(cl0, cl1, baseline)]++;
        }

        printk("%s," STR(CACHE_LINE_0_ADDR) ",%u,%u,%u,%u\n", __func__, counts[0],
               counts[1], iterations, baseline);
    }
}

void __aligned(PAGE_SIZE) test_uncond_forward_branch_cl1(unsigned iterations) {
    for (int i = 0; i < ITERATIONS; i++) {
        counts[0] = counts[1] = 0;

        for (unsigned j = 0; j < iterations; j++) {
            FLUSH_BTB();
            clflush(cl0);
            clflush(cl1);
            mfence();

            /* clang-format off */
            asm goto(
                "mov $%c[cl1], %%rsi\n"

                STR(BRANCH) " %l[end]\n"
                "mov (%%rsi), %%rax\n"
                :: [cl1] "i" (CACHE_LINE_1_ADDR)
                : "rsi", "rax", "memory"
                : end);
            /* clang-format on */
        end:
            CLEAN_STACK();
            counts[cache_channel_measure_bit(cl0, cl1, baseline)]++;
        }

        printk("%s," STR(CACHE_LINE_1_ADDR) ",%u,%u,%u,%u\n", __func__, counts[0],
               counts[1], iterations, baseline);
    }
}

int __aligned(PAGE_SIZE) test_uncond_branch_mispredictions(void *unused) {
    frame_t *frame = get_free_frame();
    channel = vmap_4k(_ptr(CHANNEL_ADDR), frame->mfn, L1_PROT);

    cl0 = &channel->lines[CACHE_LINE1]; /* CACHE_LINE_0_ADDR */
    cl1 = &channel->lines[CACHE_LINE2]; /* CACHE_LINE_1_ADDR */

    baseline = (cache_channel_baseline(cl0, 100) + cache_channel_baseline(cl1, 100)) / 2;
    baseline = min(200, baseline);

    printk("Testing direct unconditional " STR(BRANCH) " %s BTB flushing\n",
           WITH_BTB_FLUSH ? "with" : "without");

    cli();
    test_uncond_forward_branch_cl0(LOOP_ITERATIONS);
    test_uncond_forward_branch_cl1(LOOP_ITERATIONS);

    test_uncond_backward_branch_cl0(LOOP_ITERATIONS);
    test_uncond_backward_branch_cl1(LOOP_ITERATIONS);
    sti();

    return 0;
}
