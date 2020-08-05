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
#ifndef KTF_MPTABLES_H
#define KTF_MPTABLES_H

#include <ktf.h>
#include <lib.h>

#define MPF_SIGNATURE (('_' << 24) | ('P' << 16) | ('M' << 8) | ('_'))

struct mpf {
    uint32_t signature;
    uint32_t mpc_base;
    uint8_t length;
    uint8_t spec_rev;
    uint8_t checksum;
    uint8_t mpc_type;
    uint8_t rsvd0:6, imcrp:1;
    uint8_t rsvd1[3];
} __packed;
typedef struct mpf mpf_t;

#define MPC_SIGNATURE (('P' << 24) | ('M' << 16) | ('C' << 8) | ('P'))

enum mpc_entry_type {
    MPC_PROCESSOR_ENTRY,
    MPC_BUS_ENTRY,
    MPC_IOAPIC_ENTRY,
    MPC_IO_INT_ENTRY,
    MPC_LOCAL_INT_ENTRY,
};
typedef enum mpc_entry_type mpc_entry_type_t;

struct mpc_hdr {
    uint32_t signature;
    uint16_t length;
    uint8_t spec_rev;
    uint8_t checksum;
    const char oem_id[8];
    const char product_id[12];
    uint32_t oem_tlb_ptr;
    uint16_t oem_tlb_size;
    uint16_t entry_count;
    uint32_t lapic_base;
    uint16_t ext_length;
    uint8_t ext_checksum;
    uint8_t rsvd;
} __packed;
typedef struct mpc_hdr mpc_hdr_t;

struct mpc_processor_entry {
   uint8_t type;
   uint8_t lapic_id;
   uint8_t lapic_version;
   struct {
       uint8_t en:1, bsp:1, rsvd0:6;
   };
   struct {
       uint32_t stepping:4, model:4, family:4, rsvd1:20;
   };
   uint32_t feature_flags;
   uint32_t rsvd2;
   uint32_t rsvd3;
} __packed;
typedef struct mpc_processor_entry mpc_processor_entry_t;

struct mpc_bus_entry {
   uint8_t type;
   uint8_t id;
   uint8_t type_str[6];
} __packed;
typedef struct mpc_bus_entry mpc_bus_entry_t;

struct mpc_ioapic_entry {
   uint8_t type;
   uint8_t id;
   uint8_t version;
   struct {
       uint8_t en:1, rsvd:7;
   };
   uint32_t base_addr;
} __packed;
typedef struct mpc_ioapic_entry mpc_ioapic_entry_t;

#define MPC_IOINT_INT 0
#define MPC_IOINT_NMI 1
#define MPC_IOINT_SMI 2
#define MPC_IOINT_EXTINT 3

#define MPC_IOINT_POLARITY_BS 0x00
#define MPC_IOINT_POLARITY_AH 0x01
#define MPC_IOINT_POLARITY_RSVD 0x10
#define MPC_IOINT_POLARITY_AL 0x11

#define MPC_IOINT_TRIGGER_BS 0x00
#define MPC_IOINT_TRIGGER_ET 0x01
#define MPC_IOINT_TRIGGER_RSVD 0x10
#define MPC_IOINT_TRIGGER_LT 0x11

struct mpc_ioint_entry {
   uint8_t type;
   uint8_t int_type;
   struct {
       uint16_t po:1, el:1, rsvd:14;
   };
   uint8_t src_bus_id;
   uint8_t src_bus_irq;
   uint8_t dst_ioapic_id;
   uint8_t dst_ioapic_intin;
} __packed;
typedef struct mpc_ioint_entry mpc_ioint_entry_t;

struct mpc_lint_entry {
   uint8_t type;
   uint8_t int_type;
   struct {
       uint16_t po:1, el:1, rsvd:14;
   };
   uint8_t src_bus_id;
   uint8_t src_bus_irq;
   uint8_t dst_lapic_id;
   uint8_t dst_lapic_lintin;
} __packed;
typedef struct mpc_lint_entry mpc_lint_entry_t;

/* External declarations */

extern unsigned mptables_init(void);

#endif /* KTF_MPTABLES_H */
