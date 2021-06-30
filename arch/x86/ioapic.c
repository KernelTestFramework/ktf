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
#include <console.h>
#include <errno.h>
#include <ioapic.h>
#include <ktf.h>
#include <list.h>
#include <mm/slab.h>
#include <string.h>

#define IOAPIC_SYSTEM_ISA_BUS_NAME "ISA"
#define IOAPIC_SYSTEM_PCI_BUS_NAME "PCI"

/* MP specification defines bus name as array of 6 characters filled up with blanks */
#define IOAPIC_SYSTEM_BUS_NAME_SIZE 6

static ioapic_t ioapics[MAX_IOAPICS];
static unsigned nr_ioapics;

static list_head_t bus_list = LIST_INIT(bus_list);

static int __get_system_bus_name(uint8_t bus_name[IOAPIC_SYSTEM_BUS_NAME_SIZE],
                                 const char *name, size_t namelen) {
    if (namelen > IOAPIC_SYSTEM_BUS_NAME_SIZE)
        return -EINVAL;

    memset(bus_name, ' ', IOAPIC_SYSTEM_BUS_NAME_SIZE);
    memcpy(bus_name, (char *) name, namelen);
    return 0;
}

static bus_t *get_system_bus_by_name(const char *name, size_t namelen) {
    uint8_t bus_name[IOAPIC_SYSTEM_BUS_NAME_SIZE];
    bus_t *bus;

    if (__get_system_bus_name(bus_name, name, namelen) < 0)
        return NULL;

    list_for_each_entry (bus, &bus_list, list) {
        if (!memcmp(bus->name, bus_name, sizeof(bus->name)))
            return bus;
    }

    return NULL;
}

static bus_t *get_system_bus_by_id(uint8_t id) {
    bus_t *bus;

    list_for_each_entry (bus, &bus_list, list) {
        if (bus->id == id)
            return bus;
    }

    return NULL;
}

static bus_t *__add_system_bus(uint8_t id,
                               uint8_t bus_name[IOAPIC_SYSTEM_BUS_NAME_SIZE]) {
    bus_t *new_bus = kzalloc(sizeof(*new_bus));
    BUG_ON(!new_bus);

    new_bus->id = id;
    memcpy(new_bus->name, bus_name, sizeof(new_bus->name));
    list_init(&new_bus->irq_overrides);

    list_add_tail(&new_bus->list, &bus_list);
    return new_bus;
}

bus_t *add_system_bus(uint8_t id, const char *name, size_t namelen) {
    uint8_t bus_name[IOAPIC_SYSTEM_BUS_NAME_SIZE];
    bus_t *bus;

    if (__get_system_bus_name(bus_name, name, namelen) < 0) {
        printk("System Bus ID(%u) '%s' unsupported\n", id, name);
        return NULL;
    }

    bus = get_system_bus_by_id(id);
    if (bus) {
        if (memcmp(bus->name, bus_name, sizeof(bus->name))) {
            printk("System Bus ID(%u) name mismatch (actual: %6s, expected: %6s)\n", id,
                   bus->name, bus_name);
            return NULL;
        }
        return bus;
    }

    bus = __add_system_bus(id, bus_name);
    return bus;
}

int add_system_bus_irq_override(uint8_t bus_id, irq_override_t *override) {
    irq_override_t *new_override;
    bus_t *bus;

    bus = get_system_bus_by_id(bus_id);
    if (!bus)
        return -ENODEV;

    new_override = kzalloc(sizeof(*new_override));
    if (!new_override)
        return -ENOMEM;

    memcpy(new_override, override, sizeof(*new_override));
    list_add_tail(&new_override->list, &bus->irq_overrides);
    return 0;
}

static irq_override_t *__get_irq_override(bus_t *bus, uint8_t irq_type,
                                          uint32_t irq_src) {
    irq_override_t *override;

    if (!bus)
        return NULL;

    list_for_each_entry (override, &bus->irq_overrides, list) {
        if (override->type == irq_type && override->src == irq_src)
            return override;
    }

    return NULL;
}

irq_override_t *get_system_isa_bus_irq(uint8_t irq_type, uint32_t irq_src) {
    bus_t *bus;

    bus = get_system_bus_by_name(IOAPIC_SYSTEM_ISA_BUS_NAME,
                                 strlen(IOAPIC_SYSTEM_ISA_BUS_NAME));
    if (!bus)
        return NULL;

    return __get_irq_override(bus, irq_type, irq_src);
}

irq_override_t *get_system_pci_bus_irq(uint8_t irq_type, uint32_t irq_src) {
    bus_t *bus;

    bus = get_system_bus_by_name(IOAPIC_SYSTEM_PCI_BUS_NAME,
                                 strlen(IOAPIC_SYSTEM_PCI_BUS_NAME));
    if (!bus)
        return NULL;

    return __get_irq_override(bus, irq_type, irq_src);
}

void __text_init init_ioapic(void) {
    printk("Initializing IOAPICs\n");

    for (unsigned i = 0; i < nr_ioapics; i++) {
        ioapic_t *ioapic = &ioapics[i];
        ioapic_version_t version;
        ioapic_id_t id;

        id.reg = ioapic_read32(ioapic, IOAPIC_ID);
        if (ioapic->id != id.apic_id) {
            printk("IOAPIC with unexpected APIC ID detected: 0x%02x (expected: "
                   "0x%02x)\n",
                   id.apic_id, ioapic->id);
        }
        ioapic->id = id.apic_id;

        version.reg = ioapic_read32(ioapic, IOAPIC_VERSION);
        ioapic->version = version.version;
        ioapic->nr_entries = version.max_redir_entry + 1;

        for (unsigned irq = 0; irq < ioapic->nr_entries; irq++)
            set_ioapic_irq_mask(ioapic, irq, IOAPIC_INT_MASK);
    }
}

ioapic_t *get_ioapic(uint8_t id) {
    for (unsigned i = 0; i < nr_ioapics; i++) {
        if (ioapics[i].id == id)
            return &ioapics[i];
    }

    return NULL;
}

ioapic_t *add_ioapic(uint8_t id, uint8_t version, bool enabled, uint64_t base_address,
                     uint32_t gsi_base) {
    ioapic_t *ioapic;

    ioapic = get_ioapic(id);
    if (!ioapic) {
        BUG_ON(nr_ioapics > MAX_IOAPICS);
        ioapic = &ioapics[nr_ioapics++];
    }

    ioapic->id = id;
    ioapic->version = version;
    ioapic->enabled = enabled;
    ioapic->base_address = base_address;
    ioapic->gsi_base = gsi_base;

    ioapic->virt_address =
        vmap(paddr_to_virt(ioapic->base_address), paddr_to_mfn(ioapic->base_address),
             PAGE_ORDER_4K, L1_PROT);
    BUG_ON(!ioapic->virt_address);

    return ioapic;
}

int get_ioapic_redirtbl_entry(ioapic_t *ioapic, unsigned n,
                              ioapic_redirtbl_entry_t *entry) {
    ASSERT(ioapic && entry);

    if (n >= ioapic->nr_entries)
        return -EINVAL;

    entry->low = ioapic_read32(ioapic, IOAPIC_REDIRTBL(n));
    entry->high = ioapic_read32(ioapic, IOAPIC_REDIRTBL(n) + 1);

    return 0;
}

int set_ioapic_redirtbl_entry(ioapic_t *ioapic, unsigned n,
                              ioapic_redirtbl_entry_t *entry) {
    ASSERT(ioapic && entry);

    if (n >= ioapic->nr_entries)
        return -EINVAL;

    ioapic_write32(ioapic, IOAPIC_REDIRTBL(n), entry->low);
    ioapic_write32(ioapic, IOAPIC_REDIRTBL(n) + 1, entry->high);

    return 0;
}

void set_ioapic_irq_mask(ioapic_t *ioapic, unsigned irq, ioapic_int_mask_t mask) {
    ioapic_redirtbl_entry_t entry;

    get_ioapic_redirtbl_entry(ioapic, irq, &entry);
    entry.int_mask = mask;
    set_ioapic_redirtbl_entry(ioapic, irq, &entry);
}

static inline bool is_ioapic_irq(ioapic_t *ioapic, uint32_t irq_src) {
    return irq_src >= ioapic->gsi_base && irq_src < ioapic->gsi_base + ioapic->nr_entries;
}

static ioapic_t *find_ioapic_for_irq(uint32_t irq_src) {
    for (unsigned i = 0; i < nr_ioapics; i++) {
        ioapic_t *ioapic = &ioapics[i];

        if (is_ioapic_irq(ioapic, irq_src))
            return ioapic;
    }

    return NULL;
}

void configure_isa_irq(unsigned irq_src, uint8_t vector, ioapic_dest_mode_t dst_mode,
                       uint8_t dst_ids) {
    irq_override_t *irq_override = get_system_isa_bus_irq(IOAPIC_IRQ_TYPE_INT, irq_src);
    ioapic_redirtbl_entry_t entry;
    ioapic_polarity_t polarity = IOAPIC_POLARITY_AH;
    ioapic_trigger_mode_t trigger_mode = IOAPIC_TRIGGER_MODE_EDGE;
    ioapic_t *ioapic;

    if (irq_override) {
        irq_src = irq_override->dst;

        if (irq_override->dst_id == IOAPIC_DEST_ID_UNKNOWN)
            ioapic = find_ioapic_for_irq(irq_src);
        else
            ioapic = get_ioapic(irq_override->dst_id);

        if (irq_override->polarity == IOAPIC_IRQ_OVR_POLARITY_AL)
            polarity = IOAPIC_POLARITY_AL;

        if (irq_override->trigger_mode == IOAPIC_IRQ_OVR_TRIGGER_LT)
            trigger_mode = IOAPIC_TRIGGER_MODE_LEVEL;
    }
    else {
        ioapic = find_ioapic_for_irq(irq_src);
    }

    get_ioapic_redirtbl_entry(ioapic, irq_src, &entry);
    entry.vector = vector;
    entry.deliv_mode = IOAPIC_DELIVERY_MODE_FIXED;
    entry.dest_mode = dst_mode;
    entry.polarity = polarity;
    entry.trigger_mode = trigger_mode;
    entry.destination = dst_ids;
    entry.int_mask = IOAPIC_INT_UNMASK;
    set_ioapic_redirtbl_entry(ioapic, irq_src, &entry);
}
