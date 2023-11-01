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
#include <drivers/hpet.h>
#include <ioapic.h>
#include <pagetable.h>

bool init_hpet(const cpu_t *cpu) {
#ifndef KTF_ACPICA
    acpi_hpet_t *hpet;
#else
    ACPI_TABLE_HPET *hpet;
#endif
    mfn_t hpet_base_mfn;
    volatile acpi_hpet_timer_t *config;
    volatile acpi_hpet_general_t *general;
    volatile uint64_t *main_counter;
    uint64_t address;

    printk("Initializing HPET\n");

#ifndef KTF_ACPICA
    hpet = (acpi_hpet_t *) acpi_find_table(HPET_SIGNATURE);
#else
    hpet = (ACPI_TABLE_HPET *) acpi_find_table(ACPI_SIG_HPET);
#endif

    if (!hpet) {
        warning("HPET not initialized");
        return false;
    }

#ifndef KTF_ACPICA
    address = hpet->address.address;
#else
    address = hpet->Address.Address;
#endif

    hpet_base_mfn = paddr_to_mfn(address);
    vmap_kern_4k(_ptr(address), hpet_base_mfn, L1_PROT_NOCACHE_GLOB);
    config = (acpi_hpet_timer_t *) (address + HPET_OFFSET_TIMER_0_CONFIG_CAP_REG);
    general = (acpi_hpet_general_t *) (address + HPET_OFFSET_GENERAL_CAP_REG);
    main_counter = (uint64_t *) (address + HPET_OFFSET_GENERAL_MAIN_COUNTER_REG);

    general->enabled = 0;

    if (config->cfg_int_type != HPET_CONFIG_INT_TYPE_TRIGGER_MODE) {
        return false;
    }

    if (!config->cap_int_periodic) {
        return false;
    }

    if (general->num_timers_cap == 0) {
        return false;
    }

    /* Disable all timers */
    for (int i = 0; i < general->num_timers_cap; ++i) {
        (config + i)->cfg_int_enabled = 0;
    }

    *main_counter = 0;

    /* 1 fs = 10^15s */
    uint64_t freq_hz = 1000000000000000UL / general->counter_clock_period;
    uint64_t ticks = freq_hz / 1000; /* Interrupt every 1ms */

    config->cfg_int_enabled = 1;
    config->cfg_int_route = 0;
    config->cfg_fsb_enable = 0;
    config->cfg_type = 1;
    config->cfg_value_set = 1;
    config->comparator = ticks;

    general->leg_repl_cfg = 0; /* Disable legacy route */
    general->enabled = 1;

    configure_isa_irq(HPET_IRQ, HPET_IRQ_OFFSET, IOAPIC_DEST_MODE_PHYSICAL, cpu->id);
    dprintk("Initialized HPET\n");
    return true;
}
