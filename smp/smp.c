#include <ktf.h>
#include <lib.h>
#include <console.h>

#include <smp/smp.h>
#include <smp/mptables.h>

static unsigned nr_cpus;

void smp_init(void) {
    nr_cpus = mptables_init();

    printk("Initializing SMP support (CPUs: %u)\n", nr_cpus);

}
