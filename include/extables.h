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
#ifndef KTF_EXTABLES_H
#define KTF_EXTABLES_H

#include <arch/x86/processor.h>

typedef void (*extable_entry_callback_t)(cpu_regs_t *regs);

struct extable_entry {
    x86_reg_t fault_addr;
    x86_reg_t fixup;
    extable_entry_callback_t cb;
} __packed;
typedef struct extable_entry extable_entry_t;

#if defined(__x86_64__)
#define ASM_EXTABLE_HANDLER(fault_address, fixup, handler)                               \
    PUSHSECTION(.extables)                                                               \
    ".quad " STR(fault_address) ", " STR(fixup) ", " STR(handler) ";\n" POPSECTION
#else
#define ASM_EXTABLE_HANDLER(fault_address, fixup, handler)                               \
    PUSHSECTION(.extables)                                                               \
    ".long " STR(fault_address) ", " STR(fixup) ", " STR(handler) ";\n" POPSECTION
#endif

#define ASM_EXTABLE(fault_address, fixup) ASM_EXTABLE_HANDLER(fault_address, fixup, 0)

extern void init_extables(void);

#endif
