/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <ktf.h>
#include <lib.h>
#include <console.h>
#include <setup.h>
#include <apic.h>
#include <traps.h>
#include <percpu.h>

#include <mm/vmm.h>

#include <smp/smp.h>
#include <smp/mptables.h>

extern void ap_start(void);

static unsigned nr_cpus;

static unsigned ap_cpuid;
static bool ap_callin;

void __noreturn ap_startup(void) {
    write_sp(get_free_pages_top(PAGE_ORDER_2M, GFP_KERNEL));

    init_traps(ap_cpuid);

    ap_callin = true;
    smp_wmb();

    while(true)
        halt();

    UNREACHABLE();
}

static void boot_cpu(unsigned int cpu) {
    percpu_t *percpu = get_percpu_page(cpu);
    uint64_t icr;

    if (percpu->bsp)
        return;

    ap_cpuid = cpu;
    ap_callin = false;
    smp_wmb();

    dprintk("Starting AP: %u\n", cpu);

    /* Set ICR2 part */
    icr = (_ul(SET_APIC_DEST_FIELD(percpu->apic_id)) << 32);

    /* Wake up the secondary processor: INIT-SIPI-SIPI... */
    apic_wait_ready();
    apic_icr_write(icr | APIC_DM_INIT);
    apic_wait_ready();
    apic_icr_write(icr | (APIC_DM_STARTUP | GET_SIPI_VECTOR(ap_start)));
    apic_wait_ready();
    apic_icr_write(icr | (APIC_DM_STARTUP | GET_SIPI_VECTOR(ap_start)));
    apic_wait_ready();

    while(!ap_callin)
        cpu_relax();

    dprintk("AP: %u Done \n", cpu);
}

void smp_init(void) {
    nr_cpus = mptables_init();

    if (nr_cpus == 0) {
        nr_cpus = 1;
        return;
    }

    printk("Initializing SMP support (CPUs: %u)\n", nr_cpus);

    for (int i = 0; i < nr_cpus; i++)
        boot_cpu(i);
}
