/*
 * Copyright (c) 2020 Amazon.com, Inc. or its affiliates.
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
#ifndef KTF_SCHED_H
#define KTF_SCHED_H

#include <cpu.h>
#include <ktf.h>
#include <lib.h>
#include <list.h>
#include <page.h>

typedef unsigned long (*task_func_t)(void *arg);

enum task_state {
    TASK_STATE_NEW,
    TASK_STATE_READY,
    TASK_STATE_SCHEDULED,
    TASK_STATE_RUNNING,
    TASK_STATE_DONE,
};
typedef enum task_state task_state_t;

enum task_group {
    TASK_GROUP_ALL = 0,
    TASK_GROUP_ACPI,
    TASK_GROUP_TEST,
};
typedef enum task_group task_group_t;

enum task_type {
    TASK_TYPE_KERNEL = 0,
    TASK_TYPE_USER,
    TASK_TYPE_INTERRUPT,
    TASK_TYPE_ACPI_SERVICE,
};
typedef enum task_type task_type_t;

typedef unsigned int tid_t;

typedef enum task_repeat {
    TASK_REPEAT_LOOP = 0,
    TASK_REPEAT_ONCE = 1,
} task_repeat_t;

struct task {
    list_head_t list;

    tid_t id;
    task_type_t type;
    task_group_t gid;
    task_state_t state;
    task_repeat_t repeat;
    atomic64_t exec_count;

    cpu_t *cpu;
    void *stack;

    const char *name;
    task_func_t func;
    void *arg;

    unsigned long result;
};
typedef struct task task_t;

/* External declarations */

extern void init_tasks(void);
extern task_t *get_task_by_name(cpu_t *cpu, const char *name);
extern task_t *new_task(const char *name, task_func_t func, void *arg, task_type_t type);
extern int schedule_task(task_t *task, cpu_t *cpu);
extern void run_tasks(cpu_t *cpu);
extern void wait_for_task_group(const cpu_t *cpu, task_group_t group);

/* Static declarations */

static inline void set_task_group(task_t *task, task_group_t gid) {
    task->gid = gid;
}

static inline void wait_for_cpu_tasks(cpu_t *cpu) {
    wait_for_task_group(cpu, TASK_GROUP_ALL);
}

static inline task_t *new_kernel_task(const char *name, task_func_t func, void *arg) {
    return new_task(name, func, arg, TASK_TYPE_KERNEL);
}

static inline task_t *new_user_task(const char *name, task_func_t func, void *arg) {
    return new_task(name, func, arg, TASK_TYPE_USER);
}

static inline void execute_tasks(void) {
    unblock_all_cpus();
    run_tasks(get_bsp_cpu());
    wait_for_all_cpus();
}

static inline void set_task_repeat(task_t *task, task_repeat_t value) {
    ASSERT(task);
    task->repeat = value;
}

static inline void set_task_loop(task_t *task) {
    set_task_repeat(task, TASK_REPEAT_LOOP);
}

static inline void set_task_once(task_t *task) {
    set_task_repeat(task, TASK_REPEAT_ONCE);
}

#endif /* KTF_SCHED_H */
