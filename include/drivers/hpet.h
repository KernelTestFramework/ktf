
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

#ifndef KTF_HPET_H
#define KTF_HPET_H

#ifndef KTF_ACPICA
#include <acpi_ktf.h>
#else
#include <acpi.h>
#endif
#include <drivers/pit.h>

#define HPET_IRQ        PIT_IRQ
#define HPET_IRQ_OFFSET PIT_IRQ0_OFFSET

#ifndef KTF_ACPICA
#define HPET_SIGNATURE (('H') | ('P' << 8) | ('E' << 16) | ('T' << 24))

struct acpi_hpet {
    acpi_table_hdr_t header;
    uint8_t hardware_rev_id;
    uint8_t comparator_count : 5;
    uint8_t counter_size : 1;
    uint8_t reserved : 1;
    uint8_t legacy_replacement : 1;
    uint16_t pci_vendor_id;
    acpi_gas_t address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} __packed;
typedef struct acpi_hpet acpi_hpet_t;
#endif

struct acpi_hpet_timer {
    uint64_t resv0 : 1;
    uint64_t cfg_int_type : 1;
    uint64_t cfg_int_enabled : 1;
    uint64_t cfg_type : 1;
    uint64_t cap_int_periodic : 1;
    uint64_t cap_size : 1;
    uint64_t cfg_value_set : 1;
    uint64_t resv1 : 1;
    uint64_t cfg_32bit_mode : 1;
    uint64_t cfg_int_route : 5;
    uint64_t cfg_fsb_enable : 1;
    uint64_t cap_fsb_int_delivery : 1;
    uint64_t resv2 : 8;
    uint64_t cap_int_route : 32;
    uint64_t comparator;
    uint64_t fsb;
    uint64_t resv3;
};
typedef struct acpi_hpet_timer acpi_hpet_timer_t;

struct acpi_hpet_general {
    uint64_t rev_id : 8;
    uint64_t num_timers_cap : 5;
    uint64_t count_size_cap : 1;
    uint64_t resv0 : 1;
    uint64_t leg_repl_cap : 1;
    uint64_t vendor_id : 16;
    uint64_t counter_clock_period : 32;
    uint64_t _pad;
    uint64_t enabled : 1;
    uint64_t leg_repl_cfg : 1;
    uint64_t resv1 : 62;
    uint64_t _pad2;
    uint64_t timer_n_int_status : 32; /* n^th bit for the n^th timer */
    uint32_t resv2;
};
typedef struct acpi_hpet_general acpi_hpet_general_t;

enum {
    HPET_OFFSET_GENERAL_CAP_REG = 0x000,
    HPET_OFFSET_GENERAL_CONFIG_REG = 0x010,
    HPET_OFFSET_GENERAL_INT_STATUS_REG = 0x020,
    HPET_OFFSET_GENERAL_MAIN_COUNTER_REG = 0x0f0,
    HPET_OFFSET_TIMER_0_CONFIG_CAP_REG = 0x100,
    HPET_OFFSET_TIMER_0_COMPARATOR_REG = 0x108,
    HPET_OFFSET_TIMER_0_FSB_ROUTE_REG = 0x110,
};

#define HPET_CONFIG_INT_TYPE_TRIGGER_MODE 0

/* External Declarations */

extern bool init_hpet(uint8_t dst_cpus);

#endif /* KTF_HPET_H */
