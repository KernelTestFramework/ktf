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

#include <ktf.h>
#include <lib.h>
#include <list.h>
#include <page.h>

typedef void (*task_func_t)(void *arg);

enum task_state {
    TASK_STATE_NEW,
    TASK_STATE_READY,
    TASK_STATE_SCHEDULED,
    TASK_STATE_RUNNING,
    TASK_STATE_DONE,
};
typedef enum task_state task_state_t;

enum task_group {
    TASK_GROUP_UNSPECIFIED = 0,
    TASK_GROUP_TEST,
};
typedef enum task_group task_group_t;

typedef unsigned int tid_t;

struct task {
    list_head_t list;

    tid_t id;
    task_group_t gid;
    task_state_t state;

    unsigned int cpu;

    const char *name;
    task_func_t func;
    void *arg;

    unsigned long result;
} __aligned(PAGE_SIZE);
typedef struct task task_t;

/* External declarations */

extern void init_tasks(void);
extern task_t *get_task_by_id(tid_t id);
extern task_t *get_task_by_name(const char *name);
extern task_t *get_task_for_cpu(unsigned int cpu);
extern task_t *new_task(const char *name, task_func_t func, void *arg);
extern void schedule_task(task_t *task, unsigned int cpu);
extern void run_tasks(unsigned int cpu);
extern void wait_for_task_group(task_group_t group);

/* Static declarations */

static inline void set_task_group(task_t *task, task_group_t gid) { task->gid = gid; }

static inline void wait_for_all_tasks(void) {
    wait_for_task_group(TASK_GROUP_UNSPECIFIED);
}

#endif /* KTF_SCHED_H */
