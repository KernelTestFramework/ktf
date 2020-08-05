/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
