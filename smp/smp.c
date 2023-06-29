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
#include <acpi_ktf.h>
#include <apic.h>
#include <console.h>
#include <cpu.h>
#include <ktf.h>
#include <lib.h>
#include <pagetable.h>
#include <percpu.h>
#include <sched.h>
#include <setup.h>
#include <traps.h>

#include <mm/vmm.h>

#include <smp/mptables.h>
#include <smp/smp.h>

extern void ap_start(void);

static unsigned nr_cpus;

static __data_init unsigned ap_cpuid;
static __data_init bool ap_callin;
static __data_init void *ap_new_sp;
cr3_t __data_init ap_cr3;

void __noreturn ap_startup(void) {
    WRITE_SP(ap_new_sp);

    cpu_t *cpu = get_cpu(ap_cpuid);

    init_traps(cpu);
    init_apic(ap_cpuid, apic_get_mode());

    ap_callin = true;
    smp_wmb();

    init_timers(cpu);

    run_tasks(cpu);

    while (true)
        halt();

    UNREACHABLE();
}

static __text_init void boot_cpu(cpu_t *cpu) {
    percpu_t *percpu = cpu->percpu;
    apic_icr_t icr;

    if (cpu->bsp)
        return;

    ap_new_sp = get_free_pages_top(PAGE_ORDER_2M, GFP_KERNEL);
    ap_cpuid = cpu->id;
    ap_callin = false;
    smp_wmb();

    dprintk("Starting AP: %u\n", cpu->id);

    memset(&icr, 0, sizeof(icr));
    apic_icr_set_dest(&icr, percpu->apic_id);

    /* Wake up the secondary processor: INIT-SIPI-SIPI... */
    icr.deliv_mode = APIC_DELIV_MODE_INIT;
    apic_wait_ready();
    apic_icr_write(&icr);

    icr.deliv_mode = APIC_DELIV_MODE_SIPI;
    icr.vector = GET_SIPI_VECTOR(ap_start);
    apic_wait_ready();
    apic_icr_write(&icr);

    apic_wait_ready();
    apic_icr_write(&icr);

    apic_wait_ready();

    while (!ap_callin)
        cpu_relax();

    dprintk("AP: %u Done \n", cpu->id);
}

void __text_init init_smp(void) {
    nr_cpus = get_nr_cpus();

    printk("Initializing SMP support (CPUs: %u)\n", nr_cpus);
    ap_cr3 = cr3;

    for_each_cpu(boot_cpu);
}
