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
#include <ktf.h>
#include <lib.h>
#include <multiboot.h>
#include <page.h>
#include <percpu.h>
#include <setup.h>

#include <smp/mptables.h>

static unsigned nr_cpus;

static inline uint8_t get_mp_checksum(void *ptr, size_t len) {
    uint8_t checksum = 0;

    for (unsigned int i = 0; i < len; i++)
        checksum += *((uint8_t *) ptr + i);

    return checksum;
}

static inline bool validate_mpf(mpf_t *ptr) {
    if (ptr->signature != MPF_SIGNATURE)
        return false;

    /* MP Floating Pointer Structure is always 16 bytes long */
    if (ptr->length != 0x01)
        return false;

    if (ptr->spec_rev < 0x01 || ptr->spec_rev > 0x04)
        return false;

    if (get_mp_checksum(ptr, ptr->length * sizeof(*ptr)) != 0x00)
        return false;

    return true;
}

static inline mpf_t *find_mpf(void *from, void *to) {
    /* MP Floating Pointer Structure is 16 bytes aligned */
    mpf_t *addr = _ptr(_ul(from) & ~_ul(0x10));

    while (_ptr(addr) < to) {
        if (validate_mpf(addr))
            return addr;
        addr++;
    }

    return NULL;
}

static mpf_t *get_mpf_addr(void) {
    uint32_t ebda_addr;
    void *sysm_addr;
    mpf_t *ptr;

    ebda_addr = (*(uint16_t *) paddr_to_virt_kern(EBDA_ADDR_ENTRY)) << 4;
    ptr = find_mpf(paddr_to_virt_kern(ebda_addr), paddr_to_virt_kern(ebda_addr + KB(1)));
    if (ptr)
        return ptr;

    sysm_addr = paddr_to_virt_kern(get_memory_range_end(KB(512)));
    ptr = find_mpf(sysm_addr - KB(1), sysm_addr);
    if (ptr)
        return ptr;

    return find_mpf(paddr_to_virt_kern(BIOS_ROM_ADDR_START),
                    paddr_to_virt_kern(BIOS_ROM_ADDR_START + KB(64)));
}

static inline bool validate_mpc(mpc_hdr_t *ptr) {
    if (ptr->signature != MPC_SIGNATURE)
        return false;

    if (ptr->spec_rev < 0x01 || ptr->spec_rev > 0x04)
        return false;

    if (get_mp_checksum(ptr, ptr->length) != 0x00)
        return false;

    return true;
}

static mpc_hdr_t *get_mpc_addr(const mpf_t *mpf_ptr) {
    mpc_hdr_t *mpc_ptr;

    mpc_ptr = paddr_to_virt_kern(mpf_ptr->mpc_base);
    if (!validate_mpc(mpc_ptr))
        panic("Incorrect MP Configuration Table found!\n");

    return mpc_ptr;
}

static void dump_mpf(mpf_t *ptr) {
    printk("\nMP Floating Pointer Structure at %p:\n", ptr);
    printk("  Signature: %.4s\n", (char *) &ptr->signature);
    printk("  MP Configuration Table phys address: 0x%08x\n", ptr->mpc_base);
    printk("  Length: 0x%02x\n", ptr->length);
    printk("  Spec revision: 0x%02x\n", ptr->spec_rev);
    printk("  Checksum: 0x%02x\n", ptr->checksum);
    printk("  MP System Config Type: 0x%02x %s\n", ptr->mpc_type,
           ptr->mpc_type ? "(Default Config Code)" : "(MP Config Table)");
    printk("  Mode implemented: %s\n", ptr->imcrp ? "PIC Mode" : "Virtual Wire Mode");
}

static void dump_mpc_hdr(mpc_hdr_t *ptr) {
    printk("\nMP Configuration Table Header at %p:\n", ptr);
    printk("  Signature: %.4s\n", (char *) &ptr->signature);
    printk("  Length: 0x%02x\n", ptr->length);
    printk("  Spec revision: 0x%02x\n", ptr->spec_rev);
    printk("  Checksum: 0x%02x\n", ptr->checksum);
    printk("  OEM ID: %.*s\n", (int) sizeof(ptr->oem_id), ptr->oem_id);
    printk("  Product ID: %.*s\n", (int) sizeof(ptr->product_id), ptr->product_id);
    printk("  OEM Table Ptr: %p\n", _ptr(ptr->oem_tlb_ptr));
    printk("  OEM Table Size: %x\n", ptr->oem_tlb_size);
    printk("  Entry count: %x\n", ptr->entry_count);
    printk("  LAPIC base address: %p\n", _ptr(ptr->lapic_base));
    printk("  Extended Table Length: %x\n", ptr->ext_length);
    printk("  Extended Table Checksum: %x\n", ptr->ext_checksum);
}

static inline void dump_mpc_processor_entry(const mpc_processor_entry_t *e) {
    printk("  CPU: LAPIC ID=0x%02x, LAPIC version=0x%02x, Enabled=%x,"
           " BSP=%x, CPU F/M/S: 0x%02x/0x%02x/0x%02x, Features=0x%08x\n",
           e->lapic_id, e->lapic_version, e->en, e->bsp, e->family, e->model, e->stepping,
           e->feature_flags);
}

static inline void dump_mpc_bus_entry(const mpc_bus_entry_t *e) {
    printk("  BUS: ID=0x%02x Type=%.6s\n", e->id, e->type_str);
}

static inline void dump_mpc_ioapic_entry(const mpc_ioapic_entry_t *e) {
    printk("  IOAPIC: ID=%x, Version=%x, Enabled=%x, Address: %p\n", e->id, e->version,
           e->en, _ptr(e->base_addr));
}

static const char *mpc_interrupt_type_names[] = {
    [MPC_IOINT_INT] = "INT",
    [MPC_IOINT_NMI] = "NMI",
    [MPC_IOINT_SMI] = "SMI",
    [MPC_IOINT_EXTINT] = "ExtINT",
};

static const char *mpc_interrupt_polarity_names[] = {
    [MPC_IOINT_POLARITY_BS] = "Bus Spec",
    [MPC_IOINT_POLARITY_AH] = "Active High",
    [MPC_IOINT_POLARITY_RSVD] = "Reserved",
    [MPC_IOINT_POLARITY_AL] = "Active Low",
};

static const char *mpc_interrupt_trigger_names[] = {
    [MPC_IOINT_TRIGGER_BS] = "Bus Spec",
    [MPC_IOINT_TRIGGER_ET] = "Edge",
    [MPC_IOINT_TRIGGER_RSVD] = "Reserved",
    [MPC_IOINT_TRIGGER_LT] = "Level",
};

static inline void dump_mpc_ioint_entry(const mpc_ioint_entry_t *e) {
    dprintk("     IO Int: Type=%6s, Polarity=%11s, Trigger=%9s, Source Bus: "
            "ID=0x%02x IRQ=0x%02x, Dest IOAPIC: ID=0x%02x,  INTIN#=0x%02x\n",
            mpc_interrupt_type_names[e->int_type], mpc_interrupt_polarity_names[e->po],
            mpc_interrupt_trigger_names[e->el], e->src_bus_id, e->src_bus_irq,
            e->dst_ioapic_id, e->dst_ioapic_intin);
}

static inline void dump_mpc_lint_entry(const mpc_lint_entry_t *e) {
    dprintk("  Local Int: Type=%6s, Polarity=%11s, Trigger=%9s, Source Bus: "
            "ID=0x%02x IRQ=0x%02x, Dest  LAPIC: ID=0x%02x, LINTIN#=0x%02x\n",
            mpc_interrupt_type_names[e->int_type], mpc_interrupt_polarity_names[e->po],
            mpc_interrupt_trigger_names[e->el], e->src_bus_id, e->src_bus_irq,
            e->dst_lapic_id, e->dst_lapic_lintin);
}

static void process_mpc_entries(mpc_hdr_t *mpc_ptr) {
    uint8_t *entry_ptr = (uint8_t *) (mpc_ptr + 1);

    printk("\nMP Configuration Table Entries at %p:\n", entry_ptr);

    for (int i = 0; i < mpc_ptr->entry_count; i++) {
        switch (*entry_ptr) {
        case MPC_PROCESSOR_ENTRY: {
            mpc_processor_entry_t *mpc_cpu = (mpc_processor_entry_t *) entry_ptr;
            percpu_t *percpu = get_percpu_page(nr_cpus++);

            percpu->apic_id = mpc_cpu->lapic_id;
            percpu->enabled = mpc_cpu->en;
            percpu->bsp = mpc_cpu->bsp;
            percpu->family = mpc_cpu->family;
            percpu->model = mpc_cpu->model;
            percpu->stepping = mpc_cpu->stepping;

            dump_mpc_processor_entry(mpc_cpu);
            entry_ptr += sizeof(*mpc_cpu);
            break;
        }
        case MPC_BUS_ENTRY: {
            mpc_bus_entry_t *mpc_bus = (mpc_bus_entry_t *) entry_ptr;

            dump_mpc_bus_entry(mpc_bus);
            entry_ptr += sizeof(*mpc_bus);
            break;
        }
        case MPC_IOAPIC_ENTRY: {
            mpc_ioapic_entry_t *mpc_ioapic = (mpc_ioapic_entry_t *) entry_ptr;

            dump_mpc_ioapic_entry(mpc_ioapic);
            entry_ptr += sizeof(*mpc_ioapic);
            break;
        }
        case MPC_IO_INT_ENTRY: {
            mpc_ioint_entry_t *mpc_ioint = (mpc_ioint_entry_t *) entry_ptr;

            dump_mpc_ioint_entry(mpc_ioint);
            entry_ptr += sizeof(*mpc_ioint);
            break;
        }
        case MPC_LOCAL_INT_ENTRY: {
            mpc_lint_entry_t *mpc_lint = (mpc_lint_entry_t *) entry_ptr;

            dump_mpc_lint_entry(mpc_lint);
            entry_ptr += sizeof(*mpc_lint);
            break;
        }
        default:
            panic("Unknown MP Configuration Table entry type: %x\n", *entry_ptr);
        }
    }
}

unsigned mptables_init(void) {
    mpf_t *mpf_ptr = get_mpf_addr();
    mpc_hdr_t *mpc_ptr;

    if (!mpf_ptr) {
        printk("No MP Floating Structure Pointer found!\n");
        return 0;
    }

    if (opt_debug)
        dump_mpf(mpf_ptr);

    if (mpf_ptr->mpc_type > 0 || mpf_ptr->mpc_base == 0x0)
        panic("No MP Configuration Table present!\n");

    mpc_ptr = get_mpc_addr(mpf_ptr);
    if (opt_debug)
        dump_mpc_hdr(mpc_ptr);
    process_mpc_entries(mpc_ptr);

    return nr_cpus;
}
