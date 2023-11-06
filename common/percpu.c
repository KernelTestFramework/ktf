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
#include <console.h>
#include <ktf.h>
#include <lib.h>
#include <list.h>
#include <percpu.h>
#include <string.h>

#include <mm/vmm.h>

static list_head_t percpu_frames;

void init_percpu(void) {
    printk("Initialize Per CPU structures\n");

    list_init(&percpu_frames);
}

percpu_t *get_percpu_page(unsigned int cpu) {
    percpu_t *percpu;

    list_for_each_entry (percpu, &percpu_frames, list) {
        if (percpu->cpu_id == cpu)
            return percpu;
    }

    /* Per CPU page must be identity mapped,
     * because GDT descriptor has 32-bit base.
     */
    percpu = get_free_page(GFP_IDENT | GFP_KERNEL | GFP_USER);
    BUG_ON(!percpu);
    memset(percpu, 0, PAGE_SIZE);

    percpu->cpu_id = cpu;

    list_add(&percpu->list, &percpu_frames);
    return percpu;
}

void for_each_percpu(void (*func)(percpu_t *percpu)) {
    percpu_t *percpu;

    list_for_each_entry (percpu, &percpu_frames, list)
        func(percpu);
}