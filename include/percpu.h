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
#ifndef KTF_PERCPU_H
#define KTF_PERCPU_H

#include <apic.h>
#include <ktf.h>
#include <lib.h>
#include <list.h>
#include <page.h>

struct percpu {
    list_head_t list;

    unsigned int cpu_id;
    uint32_t apic_id;
    uint32_t sapic_uid;
    uint8_t sapic_id;
    uint8_t sapic_eid;
    char sapic_uid_str[1];

    apic_base_t apic_base;
    uint8_t family;
    uint8_t model;
    uint8_t stepping;

    idt_entry_t *idt __aligned(16);
    idt_ptr_t idt_ptr;

    gdt_desc_t gdt[NR_GDT_ENTRIES] __aligned(16);
    gdt_ptr_t gdt_ptr;

    x86_tss_t tss __aligned(16);
#if defined(__i386__)
    x86_tss_t tss_df __aligned(16);
#endif

    unsigned long ret2kern_sp;
    void *user_stack;
} __aligned(PAGE_SIZE);
typedef struct percpu percpu_t;

#define PERCPU_VAR(variable)    memberof(percpu_t, variable)
#define PERCPU_OFFSET(variable) ((off_t) &PERCPU_VAR(variable))
#define PERCPU_TYPE(variable)   typeof(PERCPU_VAR(variable))

#define PERCPU_GET(variable)                                                             \
    ({                                                                                   \
        PERCPU_TYPE(variable) __local_var;                                               \
        asm volatile("mov %%gs:%[percpu_var], %[local_var]"                              \
                     : [ local_var ] "=r"(__local_var)                                   \
                     : [ percpu_var ] "m"(ACCESS_ONCE(PERCPU_VAR(variable))));           \
        __local_var;                                                                     \
    })

#define PERCPU_SET_BYTE(variable, value)                                                 \
    ({                                                                                   \
        asm volatile("movb %[val], %%gs:%[percpu_var]"                                   \
                     : [ percpu_var ] "=m"(ACCESS_ONCE(PERCPU_VAR(variable)))            \
                     : [ val ] "r"((uint8_t) value));                                    \
    })

#define PERCPU_SET_WORD(variable, value)                                                 \
    ({                                                                                   \
        asm volatile("movw %[val], %%gs:%[percpu_var]"                                   \
                     : [ percpu_var ] "=m"(ACCESS_ONCE(PERCPU_VAR(variable)))            \
                     : [ val ] "r"((uint16_t) value));                                   \
    })

#define PERCPU_SET_DWORD(variable, value)                                                \
    ({                                                                                   \
        asm volatile("movl %[val], %%gs:%[percpu_var]"                                   \
                     : [ percpu_var ] "=m"(ACCESS_ONCE(PERCPU_VAR(variable)))            \
                     : [ val ] "r"((uint32_t) value));                                   \
    })

#if defined(__x86_64__)
#define PERCPU_SET_QWORD(variable, value)                                                \
    ({                                                                                   \
        asm volatile("movq %[val], %%gs:%[percpu_var]"                                   \
                     : [ percpu_var ] "=m"(ACCESS_ONCE(PERCPU_VAR(variable)))            \
                     : [ val ] "r"((uint64_t) value));                                   \
    })
#endif

#if defined(__x86_64__)
#define PERCPU_SET_MAX_SIZE sizeof(uint64_t)
#else
#define PERCPU_SET_MAX_SIZE sizeof(uint32_t)
#endif

/* clang-format off */
#define PERCPU_SET(variable, value)                                                      \
    ({                                                                                   \
        BUILD_BUG_ON(sizeof(PERCPU_TYPE(variable)) > PERCPU_SET_MAX_SIZE);               \
        size_t __var_size = sizeof(PERCPU_TYPE(variable));                               \
        if (__var_size == sizeof(uint8_t))       PERCPU_SET_BYTE(variable, value);       \
        else if (__var_size == sizeof(uint16_t)) PERCPU_SET_WORD(variable, value);       \
        else if (__var_size == sizeof(uint32_t)) PERCPU_SET_DWORD(variable, value);      \
        else if (__var_size == sizeof(uint64_t)) PERCPU_SET_QWORD(variable, value);      \
        else BUG();                                                                      \
    })
/* clang-format on */

/* External declarations */

extern void init_percpu(void);
extern percpu_t *get_percpu_page(unsigned int cpu);

#endif /* KTF_PERCPU_H */
