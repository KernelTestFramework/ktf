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
#include <ktf.h>
#include <lib.h>
#include <mm/pmm.h>
#include <page.h>
#include <pagetable.h>
#include <percpu.h>
#include <string.h>

#define ACPI_RSDP_BIOS_ROM_START 0xE0000
#define ACPI_RSDP_BIOS_ROM_STOP  0xFFFFF

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

    ebda_addr = (*(uint16_t *) paddr_to_virt_kern(EBDA_ADDR_ENTRY)) << 4;
    rsdp =
        find_rsdp(paddr_to_virt_kern(ebda_addr), paddr_to_virt_kern(ebda_addr + KB(1)));
    if (rsdp)
        return rsdp;

    rsdp = find_rsdp(paddr_to_virt_kern(ACPI_RSDP_BIOS_ROM_START),
                     paddr_to_virt_kern(ACPI_RSDP_BIOS_ROM_STOP));
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

        BUG_ON(!kmap(mfn, PAGE_ORDER_4K, L1_PROT));
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

    if (get_checksum(xsdt, tab_len) != 0x0)
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

static int process_madt_entries(void) {
    acpi_madt_t *madt = (acpi_madt_t *) acpi_find_table(MADT_SIGNATURE);
    acpi_madt_entry_t *entry;

    if (!madt)
        return -ENOENT;

    printk("ACPI: [MADT] LAPIC Addr: %p, Flags: %08x\n", _ptr(madt->lapic_addr),
           madt->flags);

    for (void *addr = madt->entry; addr < (_ptr(madt) + madt->header.length);
         addr += entry->len) {
        entry = addr;

        switch (entry->type) {
        case ACPI_MADT_TYPE_LAPIC: {
            acpi_madt_processor_t *madt_cpu = (acpi_madt_processor_t *) entry->data;
            percpu_t *percpu = get_percpu_page(madt_cpu->apic_proc_id);

            percpu->id = madt_cpu->apic_proc_id;
            percpu->apic_id = madt_cpu->apic_id;
            percpu->enabled = madt_cpu->flags & 0x1;
            if (madt_cpu->apic_proc_id == 0)
                percpu->bsp = true;

            if (!percpu->enabled)
                continue;

            nr_cpus++;
            printk("ACPI: [MADT] APIC Processor ID: %u, APIC ID: %u, Flags: %08x\n",
                   madt_cpu->apic_proc_id, madt_cpu->apic_id, madt_cpu->flags);
            break;
        }
        case ACPI_MADT_TYPE_IOAPIC:
        case ACPI_MADT_TYPE_IRQ_SRC:
        case ACPI_MADT_TYPE_NMI:
        case ACPI_MADT_TYPE_LAPIC_ADDR:
            break;
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

int init_acpi(void) {
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
    return process_madt_entries();
}
