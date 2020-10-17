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
#ifndef KTF_ACPI_H
#define KTF_ACPI_H

#include <ktf.h>
#include <lib.h>
#include <mm/pmm.h>
#include <processor.h>

#define RSDP_SIGNATURE (('R') | ('S' << 8) | ('D' << 16) | ('P' << 24))
#define RSDT_SIGNATURE (('R') | ('S' << 8) | ('D' << 16) | ('T' << 24))
#define XSDT_SIGNATURE (('X') | ('S' << 8) | ('D' << 16) | ('T' << 24))
#define MADT_SIGNATURE (('A') | ('P' << 8) | ('I' << 16) | ('C' << 24))

/* ACPI common table header */
struct acpi_table_hdr {
    uint32_t signature;
    uint32_t length;
    uint8_t rev;
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t oem_table_id[8];
    uint32_t oem_rev;
    uint8_t asl_compiler_id[4];
    uint32_t asl_compiler_rev;
} __packed;
typedef struct acpi_table_hdr acpi_table_hdr_t;

/* Root System Descriptor Pointer - Revision 1 */
struct rsdp_rev1 {
    uint64_t signature; /* ACPI signature: "RSD PTR " */
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t rev;
    uint32_t rsdt_paddr;
} __packed;
typedef struct rsdp_rev1 rsdp_rev1_t;

/* Root System Descriptor Pointer - Revision 2 */
struct rsdp_rev2 {
    rsdp_rev1_t rev1;
    uint32_t length; /* XSDT Length */
    uint64_t xsdt_paddr;
    uint8_t ext_checksum; /* Entire table checksum */
    uint8_t rsvd[3];      /* Must be 0 */
} __packed;
typedef struct rsdp_rev2 rsdp_rev2_t;

struct rsdt {
    acpi_table_hdr_t header;
    uint32_t entry[0];
} __packed;
typedef struct rsdt rsdt_t;

struct xsdt {
    acpi_table_hdr_t header;
    uint64_t entry[0];
} __packed;
typedef struct xsdt xsdt_t;

struct acpi_table {
    acpi_table_hdr_t header;
    char data[0];
} __packed;
typedef struct acpi_table acpi_table_t;

struct acpi_fadt_rev1 {
    acpi_table_hdr_t header;
    uint32_t firmware_ctrl; /* Physical address of FACS */
    uint32_t dsdt;          /* Physical address of DSDT */
    uint8_t model;
    uint8_t rsvd1;
    uint16_t sci_int; /* SCI interrupt vector */
    uint32_t smi_cmd; /* SMI command port address */
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t S4bios_req;
    uint8_t rsvd2; /* Must be zero */
    uint32_t pm1a_evt_blk;
    uint32_t pm1b_evt_blk;
    uint32_t pm1a_ctrl_blk;
    uint32_t pm1b_ctrl_blk;
    uint32_t pm2_ctrl_blk;
    uint32_t pm_tmr_blk;
    uint32_t gpe0_blk;
    uint32_t gpe1_blk;
    uint8_t pm1_evt_len;
    uint8_t pm1_ctrl_len;
    uint8_t pm2_ctrl_len;
    uint8_t pm_tmr_len;
    uint8_t gpe0_blk_len;
    uint8_t gpe1_blk_len;
    uint8_t gpe1_base;
    uint8_t rsvd3;
    uint16_t plvl2_lat;
    uint16_t plvl3_lat;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alrm;
    uint8_t mon_alrm;
    uint8_t century;
    uint8_t rsvd4[3];
} __packed;
typedef struct acpi_fadt_rev1 acpi_fadt_rev1_t;

/* Generic Address Structure */
struct acpi_gas {
    uint8_t address_space;
    uint8_t bit_width;
    uint8_t bit_offset;
    uint8_t access_size;
    uint64_t address;
};
typedef struct acpi_gas acpi_gas_t;

struct acpi_fadt_rev2 {
    acpi_fadt_rev1_t rev1;
    uint64_t x_firmware_ctrl;
    uint64_t x_dsdt;
    acpi_gas_t x_pm1a_evt_blk;
    acpi_gas_t x_pm1b_evt_blk;
    acpi_gas_t x_pm1a_ctrl_blk;
    acpi_gas_t x_pm1b_ctrl_blk;
    acpi_gas_t x_pm2_ctrl_blk;
    acpi_gas_t x_pm_tmr_blk;
    acpi_gas_t x_gpe0_blk;
    acpi_gas_t x_gpe1_blk;
} __packed;
typedef struct acpi_fadt_rev2 acpi_fadt_rev2_t;

struct acpi_madt_entry {
    uint8_t type;
    uint8_t len;
    char data[0];
} __packed;
typedef struct acpi_madt_entry acpi_madt_entry_t;

enum acpi_madt_type {
    ACPI_MADT_TYPE_LAPIC = 0,
    ACPI_MADT_TYPE_IOAPIC = 1,
    ACPI_MADT_TYPE_IRQ_SRC = 2,
    ACPI_MADT_TYPE_NMI = 4,
    ACPI_MADT_TYPE_LAPIC_ADDR = 5,
};
typedef enum acpi_madt_type acpi_madt_type_t;

struct acpi_madt_processor {
    uint8_t apic_proc_id;
    uint8_t apic_id;
    uint32_t flags;
} __packed;
typedef struct acpi_madt_processor acpi_madt_processor_t;

struct acpi_madt {
    acpi_table_hdr_t header;
    uint32_t lapic_addr;
    uint32_t flags;
    acpi_madt_entry_t entry[0];
} __packed;
typedef struct acpi_madt acpi_madt_t;

/* External Declarations */

extern acpi_table_t *acpi_find_table(uint32_t signature);

extern unsigned acpi_get_nr_cpus(void);
extern int init_acpi(void);

#endif /* KTF_ACPI_H */
