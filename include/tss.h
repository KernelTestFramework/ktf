/*
 * Copyright © 2020 Amazon.com, Inc. or its affiliates.
 * Copyright © 2014,2015 Citrix Systems Ltd.
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
#ifndef TSS_TSS_H
#define TSS_TSS_H

#ifndef __ASSEMBLY__

#if defined(__i386__)
struct __packed x86_tss32 {
    uint16_t link, rsvd0;
    uint32_t esp0;
    uint16_t ss0, rsvd1;
    uint32_t esp1;
    uint16_t ss1, rsvd2;
    uint32_t esp2;
    uint16_t ss2, rsvd3;
    uint32_t cr3;
    uint32_t _ASM_IP;
    uint32_t _ASM_FLAGS;
    uint32_t _ASM_AX;
    uint32_t _ASM_CX;
    uint32_t _ASM_DX;
    uint32_t _ASM_BX;
    uint32_t _ASM_SP;
    uint32_t _ASM_BP;
    uint32_t _ASM_SI;
    uint32_t _ASM_DI;
    uint16_t es, rsvd4;
    uint16_t cs, rsvd5;
    uint16_t ss, rsvd6;
    uint16_t ds, rsvd7;
    uint16_t fs, rsvd8;
    uint16_t gs, rsvd9;
    uint16_t ldtr, rsvd10;
    uint16_t rsvd11, iopb;
};

typedef struct x86_tss32 x86_tss_t;

#elif defined(__x86_64__)

struct __packed x86_tss64 {
    uint32_t rsvd0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t rsvd1;
    uint64_t ist[7];
    uint64_t rsvd2;
    uint16_t rsvd3, iopb;
};

typedef struct x86_tss64 x86_tss_t;

#endif /* __i386__ */

#endif /* __ASSEMBLY__ */

#endif /* TSS_TSS_H */
