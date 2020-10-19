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
#include <slab.h>
#include <string.h>

#define IOAPIC_SYSTEM_ISA_BUS_NAME "ISA"
#define IOAPIC_SYSTEM_PCI_BUS_NAME "PCI"

/* MP specification defines bus name as array of 6 characters filled up with blanks */
#define IOAPIC_SYSTEM_BUS_NAME_SIZE 6

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
    bus_t *new_bus = ktf_alloc(sizeof(*new_bus));
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

    new_override = ktf_alloc(sizeof(*new_override));
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

