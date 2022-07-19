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
#ifndef KTF_CPU_H
#define KTF_CPU_H

#include <atomic.h>
#include <ktf.h>
#include <lib.h>
#include <list.h>
#include <percpu.h>
#include <spinlock.h>

#define CPU_UNBLOCKED (1 << 0)
#define CPU_FINISHED  (1 << 1)

struct cpu {
    list_head_t list;

    unsigned int id;
    unsigned int bsp : 1, enabled : 1;

    atomic_t run_state;

    percpu_t *percpu;

    spinlock_t lock;

    list_head_t task_queue;
};
typedef struct cpu cpu_t;

/* External declarations */

extern cpu_t *init_cpus(void);
extern cpu_t *add_cpu(unsigned int id, bool bsp, bool enabled);
extern cpu_t *get_cpu(unsigned int id);
extern cpu_t *get_bsp_cpu(void);
extern unsigned int get_nr_cpus(void);
extern void for_each_cpu(void (*func)(cpu_t *cpu));
extern void unblock_all_cpus(void);
extern void wait_for_all_cpus(void);

/* Static declarations */

static inline void init_cpu_runstate(cpu_t *cpu) { atomic_set(&cpu->run_state, 0); }

static inline bool is_cpu_finished(cpu_t *cpu) {
    return atomic_test_bit(CPU_FINISHED, &cpu->run_state);
}

static inline void set_cpu_finished(cpu_t *cpu) {
    atomic_test_and_set_bit(CPU_FINISHED, &cpu->run_state);
}

static inline bool is_cpu_unblocked(cpu_t *cpu) {
    return atomic_test_bit(CPU_UNBLOCKED, &cpu->run_state);
}

static inline void set_cpu_unblocked(cpu_t *cpu) {
    atomic_test_and_set_bit(CPU_UNBLOCKED, &cpu->run_state);
}

static inline void wait_cpu_unblocked(cpu_t *cpu) {
    while (!is_cpu_unblocked(cpu))
        cpu_relax();
}

#endif /* KTF_CPU_H */
