/*
 * Copyright © 2022 Open Source Security, Inc.
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
#include <cpu.h>
#include <ktf.h>
#include <lib.h>
#include <list.h>
#include <spinlock.h>
#include <string.h>

#include <mm/slab.h>

static list_head_t cpus;
static unsigned int nr_cpus = 0;

static cpu_t bsp = {0};

static void init_cpu(cpu_t *cpu, unsigned int id, bool is_bsp, bool enabled) {
    memset(cpu, 0, sizeof(*cpu));
    cpu->id = id;
    cpu->bsp = is_bsp;
    cpu->enabled = enabled;

    init_cpu_runstate(cpu);
    if (is_bsp)
        set_cpu_unblocked(cpu);

    cpu->percpu = get_percpu_page(id);
    BUG_ON(!cpu->percpu);

    cpu->lock = SPINLOCK_INIT;
    list_init(&cpu->task_queue);
}

cpu_t *init_cpus(void) {
    printk("Initialize CPU structures\n");

    list_init(&cpus);

    init_cpu(&bsp, 0, true, true);
    list_add(&bsp.list, &cpus);
    nr_cpus = 1;

    return &bsp;
}

cpu_t *add_cpu(unsigned int id, bool is_bsp, bool enabled) {
    cpu_t *cpu = kzalloc(sizeof(*cpu));

    if (!cpu)
        return NULL;

    init_cpu(cpu, id, is_bsp, enabled);

    list_add(&cpu->list, &cpus);
    nr_cpus++;

    return cpu;
}

cpu_t *get_cpu(unsigned int id) {
    cpu_t *cpu;

    list_for_each_entry (cpu, &cpus, list) {
        if (cpu->id == id)
            return cpu;
    }

    return NULL;
}

unsigned int get_nr_cpus(void) {
    return nr_cpus;
}

cpu_t *get_bsp_cpu(void) {
    return &bsp;
}

void for_each_cpu(void (*func)(cpu_t *cpu)) {
    cpu_t *cpu;

    list_for_each_entry (cpu, &cpus, list)
        func(cpu);
}

void unblock_all_cpus(void) {
    cpu_t *cpu;

    list_for_each_entry (cpu, &cpus, list)
        set_cpu_unblocked(cpu);
}

void wait_for_all_cpus(void) {
    cpu_t *cpu, *safe;

    do {
        list_for_each_entry_safe (cpu, safe, &cpus, list) {
            if (is_cpu_finished(cpu)) {
                spin_lock(&cpu->lock);
                list_unlink(&cpu->list);
                spin_unlock(&cpu->lock);
            }
        }
    } while (!list_is_empty(&cpus));
}
