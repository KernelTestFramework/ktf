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
#include <errno.h>
#include <ioapic.h>
#include <ktf.h>
#include <lib.h>
#include <mm/pmm.h>
#include <page.h>
#include <pagetable.h>
#include <percpu.h>
#include <string.h>

acpi_table_t *acpi_tables[128];
unsigned max_acpi_tables;

static unsigned nr_cpus;

unsigned acpi_get_nr_cpus(void) { return nr_cpus; }

/* Calculate number of entries in the ACPI table.
 * Formula: (table_length - header_length) / entry_size
 */
#define ACPI_NR_TABLES(ptr)                                                              \
    (((ptr)->header.length - sizeof((ptr)->header)) / sizeof(*((ptr)->entry)))

static inline uint8_t get_checksum(void *ptr, size_t len) {
    uint8_t checksum = 0;

    for (unsigned int i = 0; i < len; i++)
        checksum += *((uint8_t *) ptr + i);

    return checksum;
}

static inline bool validate_rsdp(rsdp_rev1_t *ptr) {
    const char *rsdp_signature = "RSD PTR ";
    size_t size = 0;

    if (memcmp(rsdp_signature, &ptr->signature, sizeof(ptr->signature)))
        return false;

    switch (ptr->rev) {
    case 0: /* Revision 1 */
        size = sizeof(rsdp_rev1_t);
        break;
    case 2: /* Revision 2 */
        size = sizeof(rsdp_rev2_t);
        break;
    default:
        panic("Unknown ACPI revision: %u\n", ptr->rev);
    }

    if (get_checksum(ptr, size) != 0x0)
        return false;

    return true;
}

static inline void *find_rsdp(void *from, void *to) {
    /* RSDP structure is 16 bytes aligned */
    from = _ptr(_ul(from) & ~_ul(0x10));

    for (void *addr = from; _ptr(addr) < to; addr += 0x10) {
        rsdp_rev1_t *rsdp = addr;

        if (validate_rsdp(rsdp)) {
            printk("ACPI: RSDP [%p] v%02x %.*s\n", _ptr(virt_to_paddr(rsdp)), rsdp->rev,
                   _int(sizeof(rsdp->oem_id)), rsdp->oem_id);
            return rsdp;
        }
    }

    return NULL;
}

static rsdp_rev1_t *acpi_find_rsdp(void) {
    uint32_t ebda_addr;
    rsdp_rev1_t *rsdp;

    ebda_addr = get_bios_ebda_addr();
    rsdp =
        find_rsdp(paddr_to_virt_kern(ebda_addr), paddr_to_virt_kern(ebda_addr + KB(1)));
    if (rsdp)
        return rsdp;

    rsdp = find_rsdp(paddr_to_virt_kern(BIOS_ACPI_ROM_START),
                     paddr_to_virt_kern(BIOS_ACPI_ROM_STOP));
    if (rsdp)
        return rsdp;

    return NULL;
}

static unsigned acpi_table_map_pages(paddr_t pa, size_t len) {
    unsigned offset = pa & ~PAGE_MASK;
    unsigned num_pages = ((offset + len) / PAGE_SIZE) + 1;
    mfn_t mfn = paddr_to_mfn(pa);

    for (unsigned i = 0; i < num_pages; i++, mfn++) {
        if (mfn_invalid(mfn)) {
            panic("ACPI table at %p of length %lx has invalid MFN: %lx\n", _ptr(pa), len,
                  mfn);
        }

        BUG_ON(!kmap_4k(mfn, L1_PROT));
    }

    return num_pages;
}

static void acpi_table_unmap_pages(void *addr, unsigned mapped_pages) {
    mfn_t mfn = virt_to_mfn(addr);

    for (unsigned i = 0; i < mapped_pages; i++, mfn++) {
        kunmap(mfn_to_virt_kern(mfn), PAGE_ORDER_4K);
    }
}

static inline void acpi_dump_table(const void *tab, const acpi_table_hdr_t *hdr) {
    printk("ACPI: %.*s [%p] %04x (v%04x %.*s %04x %.*s %08x)\n",
           _int(sizeof(hdr->signature)), (char *) &hdr->signature, tab, hdr->length,
           hdr->rev, _int(sizeof(hdr->oem_id)), hdr->oem_id, hdr->oem_rev,
           _int(sizeof(hdr->asl_compiler_id)), hdr->asl_compiler_id,
           hdr->asl_compiler_rev);
}

static rsdt_t *acpi_find_rsdt(const rsdp_rev1_t *rsdp) {
    paddr_t pa = rsdp->rsdt_paddr;
    unsigned mapped_pages;
    rsdt_t *rsdt;

    mapped_pages = acpi_table_map_pages(pa, PAGE_SIZE);
    rsdt = paddr_to_virt_kern(pa);
    BUG_ON(!rsdt);

    if (RSDT_SIGNATURE != rsdt->header.signature)
        goto error;

    mapped_pages = acpi_table_map_pages(pa, rsdt->header.length);
    rsdt = paddr_to_virt_kern(pa);
    BUG_ON(!rsdt);

    if (get_checksum(rsdt, rsdt->header.length) != 0x0)
        goto error;

    acpi_dump_table(rsdt, &rsdt->header);
    return rsdt;
error:
    acpi_table_unmap_pages(rsdt, mapped_pages);
    return NULL;
}

static xsdt_t *acpi_find_xsdt(const rsdp_rev2_t *rsdp) {
    uint32_t tab_len = rsdp->length;
    paddr_t pa = rsdp->xsdt_paddr;
    unsigned mapped_pages;
    xsdt_t *xsdt;

    mapped_pages = acpi_table_map_pages(pa, tab_len);
    xsdt = paddr_to_virt_kern(pa);

    if (XSDT_SIGNATURE != xsdt->header.signature)
        goto error;

    if (get_checksum(xsdt, xsdt->header.length) != 0x0)
        goto error;

    acpi_dump_table(xsdt, &xsdt->header);
    return xsdt;
error:
    acpi_table_unmap_pages(xsdt, mapped_pages);
    return NULL;
}

static inline void acpi_dump_tables(void) {
    for (unsigned int i = 0; i < max_acpi_tables; i++)
        acpi_dump_table(acpi_tables[i], &acpi_tables[i]->header);
}

static const char *madt_int_bus_names[] = {
    [ACPI_MADT_INT_BUS_ISA] = "ISA",
};

static const char *madt_int_polarity_names[] = {
    [ACPI_MADT_INT_POLARITY_BS] = "Bus Spec",
    [ACPI_MADT_INT_POLARITY_AH] = "Active High",
    [ACPI_MADT_INT_POLARITY_RSVD] = "Reserved",
    [ACPI_MADT_INT_POLARITY_AL] = "Active Low",
};

static const char *madt_int_trigger_names[] = {
    [ACPI_MADT_INT_TRIGGER_BS] = "Bus Spec",
    [ACPI_MADT_INT_TRIGGER_ET] = "Edge",
    [ACPI_MADT_INT_TRIGGER_RSVD] = "Reserved",
    [ACPI_MADT_INT_TRIGGER_LT] = "Level",
};

static int process_madt_entries(unsigned bsp_cpu_id) {
    acpi_madt_t *madt = (acpi_madt_t *) acpi_find_table(MADT_SIGNATURE);
    acpi_madt_entry_t *entry;
    bus_t *isa_bus;

    if (!madt)
        return -ENOENT;

    printk("ACPI: [MADT] LAPIC Addr: %p, Flags: %08x\n", _ptr(madt->lapic_addr),
           madt->flags);

    isa_bus =
        add_system_bus(ACPI_MADT_INT_BUS_ISA, madt_int_bus_names[ACPI_MADT_INT_BUS_ISA],
                       strlen(madt_int_bus_names[ACPI_MADT_INT_BUS_ISA]));
    BUG_ON(!isa_bus);

    for (void *addr = madt->entry; addr < (_ptr(madt) + madt->header.length);
         addr += entry->len) {
        entry = addr;

        switch (entry->type) {
        case ACPI_MADT_TYPE_LAPIC: {
            acpi_madt_processor_t *madt_cpu = (acpi_madt_processor_t *) entry->data;
            percpu_t *percpu = get_percpu_page(madt_cpu->apic_proc_id);

            percpu->cpu_id = madt_cpu->apic_proc_id;
            percpu->apic_id = madt_cpu->apic_id;
            percpu->bsp = !!(madt_cpu->apic_proc_id == bsp_cpu_id);
            percpu->enabled = !!(madt_cpu->flags & 0x1);

            if (!percpu->enabled)
                continue;

            nr_cpus++;
            printk("ACPI: [MADT] APIC Processor ID: %u, APIC ID: %u, Flags: %08x\n",
                   madt_cpu->apic_proc_id, madt_cpu->apic_id, madt_cpu->flags);
            break;
        }
        case ACPI_MADT_TYPE_IOAPIC: {
            acpi_madt_ioapic_t *madt_ioapic = (acpi_madt_ioapic_t *) entry->data;

            add_ioapic(madt_ioapic->ioapic_id, 0x00, true, madt_ioapic->base_address,
                       madt_ioapic->gsi_base);

            printk("ACPI: [MADT] IOAPIC ID: %u, Base Address: 0x%08x, GSI Base: 0x%08x\n",
                   madt_ioapic->ioapic_id, madt_ioapic->base_address,
                   madt_ioapic->gsi_base);
            break;
        }
        case ACPI_MADT_TYPE_IRQ_SRC: {
            acpi_madt_irq_src_t *madt_irq_src = (acpi_madt_irq_src_t *) entry->data;
            irq_override_t override;

            memset(&override, 0, sizeof(override));
            override.type = ACPI_MADT_IRQ_TYPE_INT;
            override.src = madt_irq_src->irq_src;
            override.dst = madt_irq_src->gsi;
            /* Destination to be found. Each IOAPIC has GSI base and Max Redir Entry
             * register */
            override.dst_id = IOAPIC_DEST_ID_UNKNOWN;
            override.polarity = madt_irq_src->polarity;
            override.trigger_mode = madt_irq_src->trigger_mode;
            add_system_bus_irq_override(madt_irq_src->bus, &override);

            printk("ACPI: [MADT] IRQ Src Override: Bus: %3s, IRQ: 0x%02x, GSI: 0x%08x, "
                   "Polarity: %11s, Trigger: %9s\n",
                   madt_int_bus_names[madt_irq_src->bus], madt_irq_src->irq_src,
                   madt_irq_src->gsi, madt_int_polarity_names[madt_irq_src->polarity],
                   madt_int_trigger_names[madt_irq_src->trigger_mode]);
            break;
        }
        case ACPI_MADT_TYPE_NMI_SRC: {
            acpi_madt_nmi_src_t *madt_nmi_src = (acpi_madt_nmi_src_t *) entry->data;
            irq_override_t override;

            memset(&override, 0, sizeof(override));
            override.type = ACPI_MADT_IRQ_TYPE_NMI;
            override.src = madt_nmi_src->gsi;
            override.polarity = madt_nmi_src->polarity;
            override.trigger_mode = madt_nmi_src->trigger_mode;
            add_system_bus_irq_override(ACPI_MADT_INT_BUS_ISA, &override);

            printk("ACPI: [MADT] NMI Src: GSI: 0x%08x, Polarity: %11s, Trigger: %9s\n",
                   madt_nmi_src->gsi, madt_int_polarity_names[madt_nmi_src->polarity],
                   madt_int_trigger_names[madt_nmi_src->trigger_mode]);
            break;
        }
        case ACPI_MADT_TYPE_LAPIC_NMI: {
            acpi_madt_lapic_nmi_t *madt_lapic_nmi = (acpi_madt_lapic_nmi_t *) entry->data;
            irq_override_t override;

            memset(&override, 0, sizeof(override));
            override.type = ACPI_MADT_IRQ_TYPE_NMI;
            override.dst_id = madt_lapic_nmi->cpu_uid;
            override.dst = madt_lapic_nmi->lapic_lint;
            override.polarity = madt_lapic_nmi->polarity;
            override.trigger_mode = madt_lapic_nmi->trigger_mode;
            add_system_bus_irq_override(ACPI_MADT_INT_BUS_ISA, &override);

            printk("ACPI: [MADT] Local APIC NMI LINT#: CPU UID: %02x, Polarity: %11s, "
                   "Trigger: %9s, LINT#: 0x%02x\n",
                   madt_lapic_nmi->cpu_uid,
                   madt_int_polarity_names[madt_lapic_nmi->polarity],
                   madt_int_trigger_names[madt_lapic_nmi->trigger_mode],
                   madt_lapic_nmi->lapic_lint);
            break;
        }
        case ACPI_MADT_TYPE_LAPIC_ADDR: {
            acpi_madt_lapic_addr_t *madt_lapic_addr =
                (acpi_madt_lapic_addr_t *) entry->data;

            printk("ACPI: [MADT] Local APIC Address: 0x%016lx\n",
                   madt_lapic_addr->lapic_addr);
            break;
        }
        case ACPI_MADT_TYPE_IOSAPIC: {
            acpi_madt_iosapic_t *madt_iosapic = (acpi_madt_iosapic_t *) entry->data;

            add_ioapic(madt_iosapic->ioapic_id, 0x00, true, madt_iosapic->base_address,
                       madt_iosapic->gsi_base);

            printk("ACPI: [MADT] IOSAPIC ID: %u, Base Address: %p, GSI Base: 0x%08x\n",
                   madt_iosapic->ioapic_id, _ptr(madt_iosapic->base_address),
                   madt_iosapic->gsi_base);
            break;
        }
        default:
            panic("Unknown ACPI MADT entry type: %u\n", entry->type);
        }
    }

    return 0;
}

acpi_table_t *acpi_find_table(uint32_t signature) {
    for (unsigned int i = 0; i < max_acpi_tables; i++) {
        acpi_table_t *tab = acpi_tables[i];

        if (tab->header.signature == signature)
            return tab;
    }

    return NULL;
}

int init_acpi(unsigned bsp_cpu_id) {
    unsigned acpi_nr_tables;
    rsdt_t *rsdt = NULL;
    xsdt_t *xsdt = NULL;

    printk("Initializing ACPI support\n");

    rsdp_rev1_t *rsdp = acpi_find_rsdp();
    if (!rsdp)
        return -ENOENT;

    if (rsdp->rev < 2)
        rsdt = acpi_find_rsdt((rsdp_rev1_t *) rsdp);
    else
        xsdt = acpi_find_xsdt((rsdp_rev2_t *) rsdp);

    if (!rsdt && !xsdt)
        return -ENOENT;

    acpi_nr_tables = (rsdp->rev < 2) ? ACPI_NR_TABLES(rsdt) : ACPI_NR_TABLES(xsdt);

    for (unsigned int i = 0; i < acpi_nr_tables; i++) {
        paddr_t pa = (rsdt) ? rsdt->entry[i] : xsdt->entry[i];
        unsigned mapped_pages;
        acpi_table_t *tab;
        uint32_t tab_len;

        /* Map at least a header of the ACPI table */
        mapped_pages = acpi_table_map_pages(pa, PAGE_SIZE);
        tab = paddr_to_virt_kern(pa);
        BUG_ON(!tab);

        /* Find ACPI table actual length */
        tab_len = tab->header.length;

        /* Map entire ACPI table */
        mapped_pages = acpi_table_map_pages(pa, tab_len);
        tab = paddr_to_virt_kern(pa);
        BUG_ON(!tab);

        /* Verify ACPI table checksum and unmap when invalid */
        if (get_checksum(tab, tab->header.length) != 0x0)
            acpi_table_unmap_pages(tab, mapped_pages);

        acpi_tables[max_acpi_tables++] = tab;
    }

    acpi_dump_tables();
    return process_madt_entries(bsp_cpu_id);
}
