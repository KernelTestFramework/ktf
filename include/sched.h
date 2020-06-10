#ifndef KTF_SCHED_H
#define KTF_SCHED_H

#include <ktf.h>
#include <lib.h>
#include <page.h>
#include <list.h>

typedef void (*task_func_t)(void *this, void *arg);

enum task_state {
    TASK_STATE_NEW,
    TASK_STATE_READY,
    TASK_STATE_SCHEDULED,
    TASK_STATE_RUNNING,
    TASK_STATE_DONE,
};
typedef enum task_state task_state_t;

typedef unsigned int tid_t;

struct task {
    list_head_t list;

    tid_t id;
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
extern void wait_for_all_tasks(void);

#endif /* KTF_SCHED_H */
