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

#include <ktf.h>
#include <lib.h>
#include <list.h>
#include <page.h>

struct percpu {
    list_head_t  list;
    unsigned int id : 8, apic_id : 8, enabled : 1, bsp : 1, family : 4, model : 4,
        stepping : 4;

    idt_entry_t *idt __aligned(16);
    idt_ptr_t        idt_ptr;

    gdt_desc_t gdt[NR_GDT_ENTRIES] __aligned(16);
    gdt_ptr_t  gdt_ptr;

    x86_tss_t tss __aligned(16);
#if defined(__i386__)
    x86_tss_t tss_df __aligned(16);
#endif

    unsigned long ret2kern_sp;
    void *        user_stack;
} __aligned(PAGE_SIZE);
typedef struct percpu percpu_t;

/* External declarations */

extern void      init_percpu(void);
extern percpu_t *get_percpu_page(unsigned int cpu);

#endif /* KTF_PERCPU_H */
