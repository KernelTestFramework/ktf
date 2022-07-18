/*
 * Copyright © 2021 Amazon.com, Inc. or its affiliates.
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
#include <ktf.h>
#include <lib.h>

#include <extables.h>

void __noreturn halt(void) {
    cli();

    while (1) {
        hlt();
        pause();
    }
}

static uint64_t seed;

void srand(unsigned s) {
    seed = s - 1;
}

int rand(void) {
    seed = 6364136223846793005ULL * seed + 1;
    return seed >> 33;
}

/*
 * read the msr_idx msr into value; the result is valid iff the returned value is true
 */
bool rdmsr_safe(uint32_t msr_idx, uint64_t *value) {
    volatile bool success = false;
    uint32_t low, high;

    asm volatile("1: rdmsr; movb $1, %[success];"
                 "2:" ASM_EXTABLE(1b, 2b)
                 : "=a"(low), "=d"(high), [ success ] "=m"(success)
                 : "c"(msr_idx)
                 : "memory");
    *value = (((uint64_t) high) << 32) | low;
    return success;
}

/*
 * write to the msr_idx msr; the result is valid iff the returned value is true
 */
bool wrmsr_safe(uint32_t msr_idx, uint64_t value) {
    volatile bool success = false;

    asm volatile("1: wrmsr; movq $1, %[success];"
                 "2:" ASM_EXTABLE(1b, 2b)
                 : [ success ] "=m"(success)
                 : "c"(msr_idx), "a"((uint32_t) value), "d"((uint32_t)(value >> 32))
                 : "memory");
    return success;
}
