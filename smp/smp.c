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
#include <acpi.h>
#include <apic.h>
#include <console.h>
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

void __noreturn __naked ap_startup(void) {
    WRITE_SP(ap_new_sp);

    init_traps(ap_cpuid);
    init_apic(ap_cpuid, apic_get_mode());

    ap_callin = true;
    smp_wmb();

    run_tasks(smp_processor_id());

    while (true)
        halt();

    UNREACHABLE();
}

static __text_init void boot_cpu(percpu_t *percpu) {
    apic_icr_t icr;

    if (percpu->bsp)
        return;

    ap_new_sp = get_free_pages_top(PAGE_ORDER_2M, GFP_KERNEL);
    ap_cpuid = percpu->cpu_id;
    ap_callin = false;
    smp_wmb();

    dprintk("Starting AP: %u\n", percpu->cpu_id);

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

    dprintk("AP: %u Done \n", percpu->cpu_id);
}

void __text_init init_smp(void) {
    nr_cpus = acpi_get_nr_cpus();
    if (nr_cpus == 0) {
        nr_cpus = mptables_get_nr_cpus();
        if (nr_cpus == 0) {
            nr_cpus++;
            return;
        }
    }

    printk("Initializing SMP support (CPUs: %u)\n", nr_cpus);
    ap_cr3 = cr3;

    for_each_percpu(boot_cpu);
}

unsigned get_nr_cpus(void) { return nr_cpus; }
