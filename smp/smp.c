#include <ktf.h>
#include <lib.h>
#include <console.h>

#include <smp/smp.h>
#include <smp/mptables.h>

void smp_init(void) {
    printk("Initializing SMP support\n");

    mptables_init();
}
