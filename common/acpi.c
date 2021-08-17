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
#include <ioapic.h>
#include <percpu.h>
#include <setup.h>
#include <string.h>

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

static unsigned nr_cpus;

unsigned acpi_get_nr_cpus(void) { return nr_cpus; }

#ifndef KTF_ACPICA
#include <errno.h>
#include <page.h>
#include <pagetable.h>
#include <string.h>

acpi_table_t *acpi_tables[128];
unsigned max_acpi_tables;

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

static int process_fadt(void) {
    acpi_fadt_rev1_t *fadt = (acpi_fadt_rev1_t *) acpi_find_table(FADT_SIGNATURE);

    if (!fadt)
        return -1;

    boot_flags.legacy_devs = !!(fadt->boot_flags & ACPI_FADT_LEGACY_DEVICES);
    boot_flags.i8042 = !!(fadt->boot_flags & ACPI_FADT_8042);
    boot_flags.vga = !(fadt->boot_flags & ACPI_FADT_NO_VGA);

    return 0;
}

static int process_madt_entries(unsigned bsp_cpu_id) {
    acpi_madt_t *madt = (acpi_madt_t *) acpi_find_table(MADT_SIGNATURE);
    acpi_madt_entry_t *entry;
    bus_t *isa_bus = NULL;

    if (!madt)
        return -ENOENT;

    printk("ACPI: [MADT] LAPIC Addr: %p, Flags: %08x\n", _ptr(madt->lapic_addr),
           madt->flags);

    if (boot_flags.legacy_devs) {
        isa_bus = add_system_bus(ACPI_MADT_INT_BUS_ISA,
                                 madt_int_bus_names[ACPI_MADT_INT_BUS_ISA],
                                 strlen(madt_int_bus_names[ACPI_MADT_INT_BUS_ISA]));
        BUG_ON(!isa_bus);
    }

    for (void *addr = madt->entry; addr < (_ptr(madt) + madt->header.length);
         addr += entry->len) {
        entry = addr;

        switch (entry->type) {
        case ACPI_MADT_TYPE_LAPIC: {
            acpi_madt_processor_t *madt_cpu = (acpi_madt_processor_t *) entry->data;
            percpu_t *percpu;
            bool enabled;

            /* Some systems report all CPUs, marked as disabled */
            enabled = !!(madt_cpu->flags & 0x1);
            if (!enabled)
                break;

            percpu = get_percpu_page(madt_cpu->apic_proc_id);
            percpu->cpu_id = madt_cpu->apic_proc_id;
            percpu->apic_id = madt_cpu->apic_id;
            percpu->bsp = !!(madt_cpu->apic_proc_id == bsp_cpu_id);
            percpu->enabled = enabled;

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
    int rc;

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

    rc = process_fadt();
    if (rc < 0)
        return rc;

    return process_madt_entries(bsp_cpu_id);
}
#else /* KTF_ACPICA */
#include "acpi.h"

/* ACPI initialization and termination functions */

static ACPI_STATUS InitializeFullAcpi(void) {
    ACPI_STATUS status;

    /* Initialize the ACPICA subsystem */
    status = AcpiInitializeSubsystem();
    if (ACPI_FAILURE(status))
        return status;

    /* Initialize the ACPICA Table Manager and get all ACPI tables */
    status = AcpiInitializeTables(NULL, 16, true);
    if (ACPI_FAILURE(status))
        return status;

    /* Create the ACPI namespace from ACPI tables */
    status = AcpiLoadTables();
    if (ACPI_FAILURE(status))
        return status;

    /* Note: Local handlers should be installed here */
    /* Initialize the ACPI hardware */
    status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE(status))
        return status;

    /* Complete the ACPI namespace object initialization */
    status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE(status))
        return status;

    return AE_OK;
}

static void madt_parser(ACPI_SUBTABLE_HEADER *entry, void *arg) {
    bus_t *isa_bus =
        add_system_bus(ACPI_MADT_INT_BUS_ISA, madt_int_bus_names[ACPI_MADT_INT_BUS_ISA],
                       strlen(madt_int_bus_names[ACPI_MADT_INT_BUS_ISA]));

    BUG_ON(!isa_bus);

    switch (entry->Type) {
    case ACPI_MADT_TYPE_LOCAL_APIC: {
        ACPI_MADT_LOCAL_APIC *lapic = (ACPI_MADT_LOCAL_APIC *) entry;
        uint32_t bsp_cpu_id = (uint32_t) _ul(arg);
        percpu_t *percpu;
        bool enabled;

        /* Some systems report all CPUs, marked as disabled */
        enabled = !!(lapic->LapicFlags & 0x1);
        if (!enabled)
            break;

        percpu = get_percpu_page(lapic->ProcessorId);
        percpu->cpu_id = lapic->ProcessorId;
        percpu->apic_id = lapic->Id;
        percpu->bsp = !!(lapic->ProcessorId == bsp_cpu_id);
        percpu->enabled = enabled;

        nr_cpus++;
        printk("ACPI: [MADT] APIC Processor ID: %u, APIC ID: %u, Flags: %08x\n",
               percpu->cpu_id, percpu->apic_id, lapic->LapicFlags);
        break;
    }
    case ACPI_MADT_TYPE_IO_APIC: {
        ACPI_MADT_IO_APIC *ioapic = (ACPI_MADT_IO_APIC *) entry;

        add_ioapic(ioapic->Id, 0x00, true, ioapic->Address, ioapic->GlobalIrqBase);

        printk("ACPI: [MADT] IOAPIC ID: %u, Base Address: 0x%08x, GSI Base: 0x%08x\n",
               ioapic->Id, ioapic->Address, ioapic->GlobalIrqBase);

        break;
    }
    case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE: {
        ACPI_MADT_INTERRUPT_OVERRIDE *irq_src = (ACPI_MADT_INTERRUPT_OVERRIDE *) entry;
        irq_override_t override;

        memset(&override, 0, sizeof(override));
        override.type = ACPI_MADT_IRQ_TYPE_INT;
        override.src = irq_src->SourceIrq;
        override.dst = irq_src->GlobalIrq;
        /* Destination to be found. Each IOAPIC has GSI base and Max Redir Entry
         * register */
        override.dst_id = IOAPIC_DEST_ID_UNKNOWN;

        inti_flags_t flags = (inti_flags_t) irq_src->IntiFlags;
        override.polarity = flags.polarity;
        override.trigger_mode = flags.trigger_mode;
        add_system_bus_irq_override(irq_src->Bus, &override);

        printk("ACPI: [MADT] IRQ Src Override: Bus: %3s, IRQ: 0x%02x, GSI: 0x%08x, "
               "Polarity: %11s, Trigger: %9s\n",
               madt_int_bus_names[irq_src->Bus], irq_src->SourceIrq, irq_src->GlobalIrq,
               madt_int_polarity_names[flags.polarity],
               madt_int_trigger_names[flags.trigger_mode]);
        break;
    }
    case ACPI_MADT_TYPE_NMI_SOURCE: {
        ACPI_MADT_NMI_SOURCE *nmi_src = (ACPI_MADT_NMI_SOURCE *) entry;
        irq_override_t override;

        memset(&override, 0, sizeof(override));
        override.type = ACPI_MADT_IRQ_TYPE_NMI;
        override.src = nmi_src->GlobalIrq;

        inti_flags_t flags = (inti_flags_t) nmi_src->IntiFlags;
        override.polarity = flags.polarity;
        override.trigger_mode = flags.trigger_mode;
        add_system_bus_irq_override(ACPI_MADT_INT_BUS_ISA, &override);

        printk("ACPI: [MADT] NMI Src: GSI: 0x%08x, Polarity: %11s, Trigger: %9s\n",
               nmi_src->GlobalIrq, madt_int_polarity_names[flags.polarity],
               madt_int_trigger_names[flags.trigger_mode]);
        break;
    }
    case ACPI_MADT_TYPE_LOCAL_APIC_NMI: {
        ACPI_MADT_LOCAL_APIC_NMI *lapic_nmi = (ACPI_MADT_LOCAL_APIC_NMI *) entry;
        irq_override_t override;

        memset(&override, 0, sizeof(override));
        override.type = ACPI_MADT_IRQ_TYPE_NMI;
        override.dst_id = lapic_nmi->ProcessorId;
        override.dst = lapic_nmi->Lint;

        inti_flags_t flags = (inti_flags_t) lapic_nmi->IntiFlags;
        override.polarity = flags.polarity;
        override.trigger_mode = flags.trigger_mode;
        add_system_bus_irq_override(ACPI_MADT_INT_BUS_ISA, &override);

        printk("ACPI: [MADT] Local APIC NMI LINT#: CPU UID: %02x, Polarity: %11s, "
               "Trigger: %9s, LINT#: 0x%02x\n",
               lapic_nmi->ProcessorId, madt_int_polarity_names[flags.polarity],
               madt_int_trigger_names[flags.trigger_mode], lapic_nmi->Lint);
        break;
    }
    case ACPI_MADT_TYPE_LOCAL_APIC_OVERRIDE: {
        ACPI_MADT_LOCAL_APIC_OVERRIDE *lapic_addr =
            (ACPI_MADT_LOCAL_APIC_OVERRIDE *) entry;

        /* FIXME: set for each Per-CPU apic_base */
        printk("ACPI: [MADT] Local APIC Address: 0x%016lx\n", lapic_addr->Address);
        break;
    }
    case ACPI_MADT_TYPE_IO_SAPIC: {
        ACPI_MADT_IO_SAPIC *iosapic = (ACPI_MADT_IO_SAPIC *) entry;

        add_ioapic(iosapic->Id, 0x00, true, iosapic->Address, iosapic->GlobalIrqBase);

        printk("ACPI: [MADT] IOSAPIC ID: %u, Base Address: %p, GSI Base: 0x%08x\n",
               iosapic->Id, _ptr(iosapic->Address), iosapic->GlobalIrqBase);
        break;
    }
    case ACPI_MADT_TYPE_LOCAL_SAPIC: {
        ACPI_MADT_LOCAL_SAPIC *slapic = (ACPI_MADT_LOCAL_SAPIC *) entry;
        percpu_t *percpu = get_percpu_page(slapic->ProcessorId);
        uint32_t bsp_cpu_id = (uint32_t) _ul(arg);

        percpu->cpu_id = slapic->ProcessorId;
        percpu->sapic_id = slapic->Id;
        percpu->sapic_eid = slapic->Eid;

        percpu->sapic_uid = slapic->Uid;
        percpu->sapic_uid_str[0] = slapic->UidString[0];

        percpu->bsp = !!(slapic->ProcessorId == bsp_cpu_id);
        percpu->enabled = !!(slapic->LapicFlags & 0x1);

        if (percpu->enabled) {
            nr_cpus++;
            printk("ACPI: [MADT] SAPIC Processor ID: %u, SAPIC ID: %u, SAPIC EID: %u, "
                   "SAPIC UID: %u, SAPIC UID Str: %c Flags: %08x\n",
                   percpu->cpu_id, slapic->Id, slapic->Eid, slapic->Uid,
                   slapic->UidString[0], slapic->LapicFlags);
        }
        break;
    }
    case ACPI_MADT_TYPE_INTERRUPT_SOURCE: {
        /* TODO: to be implemented */
        break;
    }
    case ACPI_MADT_TYPE_LOCAL_X2APIC: {
        ACPI_MADT_LOCAL_X2APIC *x2lapic = (ACPI_MADT_LOCAL_X2APIC *) entry;
        percpu_t *percpu = get_percpu_page(x2lapic->Uid);
        uint32_t bsp_cpu_id = (uint32_t) _ul(arg);

        percpu->cpu_id = x2lapic->Uid;
        percpu->apic_id = x2lapic->LocalApicId;
        percpu->bsp = !!(x2lapic->Uid == bsp_cpu_id);
        percpu->enabled = !!(x2lapic->LapicFlags & 0x1);

        if (percpu->enabled) {
            nr_cpus++;
            printk("ACPI: [MADT] X2APIC Processor ID: %u, APIC ID: %u, Flags: %08x\n",
                   percpu->cpu_id, percpu->apic_id, x2lapic->LapicFlags);
        }
        break;
    }
    case ACPI_MADT_TYPE_LOCAL_X2APIC_NMI: {
        ACPI_MADT_LOCAL_X2APIC_NMI *x2lapic_nmi = (ACPI_MADT_LOCAL_X2APIC_NMI *) entry;
        irq_override_t override;

        memset(&override, 0, sizeof(override));
        override.type = ACPI_MADT_IRQ_TYPE_NMI;
        override.dst_id = x2lapic_nmi->Uid;
        override.dst = x2lapic_nmi->Lint;

        inti_flags_t flags = (inti_flags_t) x2lapic_nmi->IntiFlags;
        override.polarity = flags.polarity;
        override.trigger_mode = flags.trigger_mode;
        add_system_bus_irq_override(ACPI_MADT_INT_BUS_ISA, &override);

        printk("ACPI: [MADT] Local X2APIC NMI LINT#: CPU UID: %02x, Polarity: %11s, "
               "Trigger: %9s, LINT#: 0x%02x\n",
               x2lapic_nmi->Uid, madt_int_polarity_names[flags.polarity],
               madt_int_trigger_names[flags.trigger_mode], x2lapic_nmi->Lint);
        break;
    }
    case ACPI_MADT_TYPE_GENERIC_INTERRUPT: {
        /* TODO: to be implemented */
        break;
    }
    case ACPI_MADT_TYPE_GENERIC_DISTRIBUTOR: {
        /* TODO: to be implemented */
        break;
    }
    case ACPI_MADT_TYPE_GENERIC_MSI_FRAME: {
        /* TODO: to be implemented */
        break;
    }
    case ACPI_MADT_TYPE_GENERIC_REDISTRIBUTOR: {
        /* TODO: to be implemented */
        break;
    }
    case ACPI_MADT_TYPE_GENERIC_TRANSLATOR: {
        /* TODO: to be implemented */
        break;
    }
    case ACPI_MADT_TYPE_MULTIPROC_WAKEUP: {
        /* TODO: to be implemented */
        break;
    }
    default:
        panic("ACPI [MADT]: Unsupported subtable entry type: %x\n", entry->Type);
        break;
    }
}

static ACPI_STATUS init_fadt(void) {
    ACPI_TABLE_FADT *fadt = acpi_find_table(ACPI_SIG_FADT);

    if (!fadt)
        return AE_ERROR;

    boot_flags.legacy_devs = !!(fadt->BootFlags & ACPI_FADT_LEGACY_DEVICES);
    boot_flags.i8042 = !!(fadt->BootFlags & ACPI_FADT_8042);
    boot_flags.vga = !(fadt->BootFlags & ACPI_FADT_NO_VGA);

    return AE_OK;
}

static ACPI_STATUS init_madt(unsigned bsp_cpu_id) {
    ACPI_TABLE_MADT *madt = acpi_find_table(ACPI_SIG_MADT);
    ACPI_SUBTABLE_HEADER *subtbl = (void *) madt + sizeof(*madt);
    uint32_t length = madt->Header.Length - sizeof(*madt);

    if (!madt || !subtbl)
        return AE_ERROR;

    acpi_walk_subtables(subtbl, length, madt_parser, (void *) _ul(bsp_cpu_id));
    return AE_OK;
}

void *acpi_find_table(char *signature) {
    ACPI_TABLE_HEADER *hdr;

    AcpiGetTable(signature, 1, &hdr);
    return hdr;
}

void acpi_walk_subtables(ACPI_SUBTABLE_HEADER *entry, uint32_t length,
                         acpi_subtable_parser_t parser, void *arg) {
    ACPI_SUBTABLE_HEADER *stop = (void *) entry + length;

    while (entry < stop && entry->Length >= sizeof(*entry)) {
        parser(entry, arg);
        entry = ACPI_ADD_PTR(ACPI_SUBTABLE_HEADER, entry, entry->Length);
    }
}

ACPI_STATUS init_acpi(unsigned bsp_cpu_id) {
    ACPI_STATUS status;

    printk("Initializing ACPI support\n");

    status = InitializeFullAcpi();
    if (status != AE_OK)
        return status;

    status = init_fadt();
    if (status != AE_OK)
        return status;

    status = init_madt(bsp_cpu_id);
    return status;
}

void acpi_power_off(void) {
    AcpiEnterSleepStatePrep(ACPI_STATE_S5);
    cli();
    AcpiEnterSleepState(ACPI_STATE_S5);
    panic("Power Off");
}
#endif /* KTF_ACPICA */
