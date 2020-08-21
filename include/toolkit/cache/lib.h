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
#ifndef KTF_TOOLKIT_CACHE_H
#define KTF_TOOLKIT_CACHE_H

#include <lib.h>
#include <page.h>

#define CACHE_LINE_SIZE     64
#define CACHE_CHANNEL_LINES 64
#define CACHE_LINE1         31 /* cache line offset for trigger function 1 */
#define CACHE_LINE2         (2 * CACHE_LINE1) /* cache line offset for function 2 */

#define SAMPLES_COUNT 64

typedef enum { H_NULL, H_ONE, H_UNDECIDED, H_EMPTY } heisenbit_t;

struct cache_line {
    uint8_t m8[CACHE_LINE_SIZE];
} __packed __aligned(CACHE_LINE_SIZE);
typedef struct cache_line cache_line_t;

typedef struct cache_channel {
    cache_line_t lines[CACHE_CHANNEL_LINES] __aligned(PAGE_SIZE);
} cache_channel_t;

/* Static declarations */

static inline void cache_read_access_barrier(const volatile void *p) {
    asm volatile("mfence;"
                 "mov %0, %%rax;"
                 "mfence;"
                 :
                 : "m"(*(volatile unsigned long const *) p)
                 : "rax", "memory");
}

static inline void cache_write_access_barrier(volatile void *p) {
    asm volatile("mfence;"
                 "mov %%rax, %0;"
                 "mfence;"
                 :
                 : "m"(*(volatile unsigned long *) p)
                 : "memory");
}

static inline void cache_read_access(const volatile void *p) {
    asm volatile("mov %0, %%rax;"
                 :
                 : "m"(*(volatile unsigned long const *) p)
                 : "rax", "memory");
}

static inline void cache_write_access(void *p) {
    asm volatile("mov %%rax, %0" : : "m"(*(volatile unsigned long *) p) : "memory");
}

/* Get number of cycles needed to read from specified memory address. */
static inline uint32_t cache_read_access_time(const volatile void *p) {
    uint32_t ret;

    asm volatile("mfence;"
                 "rdtscp;"
                 "mov %%eax, %%ebx;"
                 "mov %1, %%eax;"
                 "lfence;"
                 "rdtscp;"
                 "sub %%ebx, %%eax;"
                 "mov %%eax, %0;"
                 : "=r"(ret)
                 : "m"(*(volatile unsigned long const *) p)
                 : "rax", "rbx", "rcx", "rdx", "memory");

    return ret;
}

/* Get number of cycles needed to write to specified memory address. */
static inline uint32_t cache_write_access_time(volatile void *p) {
    uint32_t ret;

    asm volatile("mfence;"
                 "rdtscp;"
                 "mov %%eax, %%ebx;"
                 "mov %%eax, %1;"
                 "lfence;"
                 "rdtscp;"
                 "sub %%ebx, %%eax;"
                 "mov %%eax, %0;"
                 : "=r"(ret)
                 : "m"(*(volatile unsigned long *) p)
                 : "rax", "rbx", "rcx", "rdx", "memory");

    return ret;
}

/*
 * Return cache channel state of the bit by checking cache lines presence.
 * Cache channel uses 2 cache lines (i.e. 2 bits of information) to leak 1 bit of data.
 * Presence of first cache line (cl1) is assigned to secret bit's value of 0.
 * Presence of second cache line (cl2) is assigned to secret bit's value of 1.
 * When both cache lines are present at the same time, most likely the cache access
 * baseline value (sep_time) is incorrect.
 */
static inline heisenbit_t cache_channel_measure_bit(const void *cl1, const void *cl2,
                                                    unsigned sep_time) {
    uint64_t t1 = cache_read_access_time(cl1);
    uint64_t t2 = cache_read_access_time(cl2);

    if (t2 < sep_time && t1 > sep_time)
        return H_ONE;
    else if (t1 < sep_time && t2 > sep_time)
        return H_NULL;
    else if (t1 < sep_time && t2 < sep_time)
        return H_UNDECIDED;
    else
        return H_EMPTY;
}

/* External declarations */

extern uint64_t cache_channel_baseline(const cache_line_t *cl, unsigned delay);

#endif /* KTF_TOOLKIT_CACHE_H */
