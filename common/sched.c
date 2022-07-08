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
#include <console.h>
#include <errno.h>
#include <ktf.h>
#include <lib.h>
#include <list.h>
#include <sched.h>
#include <setup.h>
#include <spinlock.h>
#include <string.h>

#include <smp/smp.h>

#include <mm/slab.h>

static list_head_t tasks;
static tid_t next_tid;

static spinlock_t lock = SPINLOCK_INIT;

void init_tasks(void) {
    printk("Initializing tasks\n");

    list_init(&tasks);
}

static const char *task_state_names[] = {
    [TASK_STATE_NEW] = "NEW",
    [TASK_STATE_READY] = "READY",
    [TASK_STATE_SCHEDULED] = "SCHEDULED",
    [TASK_STATE_RUNNING] = "RUNNING",
    [TASK_STATE_DONE] = "DONE",
};

#define PAGE_ORDER_TASK PAGE_ORDER_4K

static inline void set_task_state(task_t *task, task_state_t state) {
    ASSERT(task);

    dprintk("CPU[%u]: state transition %s -> %s\n", task->cpu,
            task_state_names[task->state], task_state_names[state]);

    ACCESS_ONCE(task->state) = state;
    smp_mb();
}

static inline task_state_t get_task_state(task_t *task) {
    task_state_t state;
    ASSERT(task);

    state = ACCESS_ONCE(task->state);
    smp_rmb();

    return state;
}

static task_t *create_task(void) {
    task_t *task = kzalloc(sizeof(*task));

    if (!task)
        return NULL;

    memset(task, 0, sizeof(*task));
    task->id = next_tid++;
    task->gid = TASK_GROUP_ALL;
    task->cpu = INVALID_CPU;
    set_task_state(task, TASK_STATE_NEW);

    spin_lock(&lock);
    list_add(&task->list, &tasks);
    spin_unlock(&lock);

    return task;
}

/* The caller should never use the parameter again after calling this function */
static void destroy_task(task_t *task) {
    if (!task)
        return;

    spin_lock(&lock);
    list_unlink(&task->list);
    spin_unlock(&lock);

    kfree(task);
}

static int prepare_task(task_t *task, const char *name, task_func_t func, void *arg,
                        task_type_t type) {
    if (!task)
        return -EINVAL;

    if (get_task_by_name(name))
        return -EEXIST;

    BUG_ON(get_task_state(task) > TASK_STATE_READY);

    task->name = name;
    task->func = func;
    task->arg = arg;
    task->type = type;
    set_task_state(task, TASK_STATE_READY);
    return ESUCCESS;
}

static void wait_for_task_state(task_t *task, task_state_t state) {
    if (!task)
        return;

    while (get_task_state(task) != state)
        cpu_relax();
}

task_t *new_task(const char *name, task_func_t func, void *arg, task_type_t type) {
    task_t *task = create_task();

    if (!task)
        return NULL;

    if (unlikely(prepare_task(task, name, func, arg, type) != ESUCCESS)) {
        destroy_task(task);
        return NULL;
    }

    return task;
}

task_t *get_task_by_id(tid_t id) {
    task_t *task;

    list_for_each_entry (task, &tasks, list) {
        if (task->id == id)
            return task;
    }

    return NULL;
}

task_t *get_task_by_name(const char *name) {
    task_t *task;

    list_for_each_entry (task, &tasks, list) {
        if (string_equal(task->name, name))
            return task;
    }

    return NULL;
}

task_t *get_task_for_cpu(unsigned int cpu) {
    task_t *task;

    list_for_each_entry (task, &tasks, list) {
        if (task->cpu == cpu)
            return task;
    }

    return NULL;
}

void schedule_task(task_t *task, unsigned int cpu) {
    ASSERT(task);

    if (cpu > get_nr_cpus() - 1)
        panic("CPU[%u] does not exist.\n", cpu);

    BUG_ON(get_task_state(task) != TASK_STATE_READY);

    printk("CPU[%u]: Scheduling task %s[%u]\n", cpu, task->name, task->id);

    task->cpu = cpu;
    set_task_state(task, TASK_STATE_SCHEDULED);
}

static void run_task(task_t *task) {
    if (!task)
        return;

    wait_for_task_state(task, TASK_STATE_SCHEDULED);

    printk("CPU[%u]: Running task %s[%u]\n", task->cpu, task->name, task->id);

    set_task_state(task, TASK_STATE_RUNNING);
    task->result = task->func(task->arg);
    set_task_state(task, TASK_STATE_DONE);
}

void wait_for_task_group(task_group_t group) {
    task_t *task;
    bool busy;

    do {
        busy = false;

        list_for_each_entry (task, &tasks, list) {
            /* When group is unspecified the functions waits for all tasks. */
            if (group != TASK_GROUP_ALL && task->gid != group)
                continue;

            if (get_task_state(task) != TASK_STATE_DONE) {
                busy = true;
                wait_for_task_state(task, TASK_STATE_DONE);
            }
        }
        cpu_relax();
    } while (busy);
}

void run_tasks(unsigned int cpu) {
    task_t *task;

    while ((task = get_task_for_cpu(cpu))) {
        switch (task->state) {
        case TASK_STATE_DONE:
            printk("Task '%s' finished with result %lu\n", task->name, task->result);
            destroy_task(task);
            break;
        case TASK_STATE_SCHEDULED:
            run_task(task);
            break;
        default:
            break;
        }
        cpu_relax();
    }
}
