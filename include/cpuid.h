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
#ifndef KTF_CPUID_H
#define KTF_CPUID_H

#include <string.h>

/* CPU vendor detection */
#define CPUID_EXT_INFO_LEAF  0x80000000U
#define CPUID_BRAND_INFO_MIN 0x80000002U
#define CPUID_BRAND_INFO_MAX 0x80000004U

static inline bool cpu_vendor_string(char *cpu_str) {
    uint32_t leaf = CPUID_EXT_INFO_LEAF;
    uint32_t ebx = 0, ecx = 0, edx = 0;
    uint32_t eax = cpuid_eax(leaf);

    if (!(eax & leaf) || (eax < CPUID_BRAND_INFO_MAX)) {
        dprintk("Extended Function CPUID Information not supported\n");
        return false;
    }

    for (leaf = CPUID_BRAND_INFO_MIN; leaf <= CPUID_BRAND_INFO_MAX;
         leaf++, cpu_str += 16) {
        cpuid(leaf, &eax, &ebx, &ecx, &edx);
        memcpy(cpu_str, &eax, sizeof(eax));
        memcpy(cpu_str + 4, &ebx, sizeof(ebx));
        memcpy(cpu_str + 8, &ecx, sizeof(ecx));
        memcpy(cpu_str + 12, &edx, sizeof(edx));
    }

    return true;
}

#endif /* KTF_CPUID_H */