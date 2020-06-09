#include <ktf.h>
#include <lib.h>
#include <list.h>
#include <console.h>
#include <percpu.h>

#include <mm/vmm.h>

static list_head_t percpu_frames;

void init_percpu(void) {
    printk("Initialize Per CPU structures\n");

    list_init(&percpu_frames);
}

percpu_t *get_percpu_page(unsigned int cpu) {
    percpu_t *percpu;

    list_for_each_entry(percpu, &percpu_frames, list) {
        if (percpu->id == cpu)
            return percpu;
    }

    /* Per CPU page must be identity mapped,
     * because GDT descriptor has 32-bit base.
     */
    percpu = get_free_page(GFP_IDENT | GFP_KERNEL);
    BUG_ON(!percpu);

    percpu->id = cpu;
    list_add(&percpu->list, &percpu_frames);
    return percpu;
}
