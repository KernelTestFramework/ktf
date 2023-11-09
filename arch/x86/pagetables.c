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
#include <multiboot2.h>
#include <page.h>
#include <pagetable.h>
#include <setup.h>
#include <spinlock.h>
#include <string.h>

static uint8_t _tmp_mapping[PAGE_SIZE] __aligned(PAGE_SIZE);
static pgentry_t *_tmp_mapping_entry;

cr3_t __aligned(PAGE_SIZE) cr3;
cr3_t user_cr3;

/* Used by lower level vmap() functions - must not be taken before mmap_lock */
static spinlock_t vmap_lock = SPINLOCK_INIT;

static inline void *tmp_map_mfn(mfn_t mfn) {
    BUG_ON(mfn_invalid(mfn));
    set_pgentry(_tmp_mapping_entry, mfn, L1_PROT);
    invlpg(_tmp_mapping);
    return _tmp_mapping;
}

static inline const char *dump_pte_flags(char *buf, size_t size, pte_t pte) {
    /* clang-format off */
    snprintf(buf, size, "%c %c%c%c%c%c%c%c%c%c",
        pte.NX  ? '-' : 'X',
        pte.G   ? 'G' : '-',
        pte.PAT ? 'p' : '-',
        pte.D   ? 'D' : '-',
        pte.A   ? 'A' : '-',
        pte.PCD ? '-' : 'C',
        pte.PWT ? 'w' : '-',
        pte.US  ? 'U' : 'S',
        pte.RW  ? 'W' : 'R',
        pte.P   ? 'P' : '-');
    /* clang-format on */

    return buf;
}

static inline int level_to_entries(int level) {
    switch (level) {
    case 4:
        return L4_PT_ENTRIES;
    case 3:
        return L3_PT_ENTRIES;
    case 2:
        return L2_PT_ENTRIES;
    case 1:
        return L1_PT_ENTRIES;
    default:
        return 0;
    }
}

static inline void dump_pte(void *entry, mfn_t table, int level, int index) {
    pte_t *pte = entry;
    paddr_t pt_paddr = mfn_to_paddr(table) + index * sizeof(pgentry_t);
    paddr_t paddr = mfn_to_paddr(pte->mfn);
    int indent = (4 - level) * 2;
    char flags[16];

    dump_pte_flags(flags, sizeof(flags), *pte);
    printk("[0x%lx] %*s%d[%03u] paddr: 0x%016lx flags: %s\n", pt_paddr, indent, "L",
           level, index, paddr, flags);
}

static inline bool is_canon_va(const void *va) {
    const unsigned int sign_bits = BITS_PER_LONG - VA_BITS;
    return _ptr(((long) va << sign_bits) >> sign_bits) == va;
}

static void dump_pagetable(mfn_t table, int level) {
    pte_t *pt;

    BUG_ON(mfn_invalid(table));
    pt = tmp_map_mfn(table);
    BUG_ON(!pt);

    for (int i = 0; i < level_to_entries(level); i++) {
        if (!pt[i].P)
            continue;

        dump_pte(&pt[i], table, level, i);

        if (level == 2 && ((pde_t *) pt)[i].PS)
            continue;
        if (level == 3 && ((pdpe_t *) pt)[i].PS)
            continue;
        dump_pagetable(pt[i].mfn, level - 1);
        pt = tmp_map_mfn(table);
    }
}

void dump_pagetables(cr3_t *cr3_ptr) {
#if defined(__x86_64__)
    int level = 4;
#else
    int level = 3;
#endif
    ASSERT(cr3_ptr);
    if (mfn_invalid(cr3_ptr->mfn)) {
        warning("CR3: 0x%lx is invalid", cr3.paddr);
        return;
    }

    printk("Page Tables: CR3 paddr: 0x%lx\n", cr3.paddr);
    spin_lock(&vmap_lock);
    dump_pagetable(cr3_ptr->mfn, level);
    spin_unlock(&vmap_lock);
}

static void dump_pagetable_va(cr3_t *cr3_ptr, void *va) {
    paddr_t tab_paddr;
    pgentry_t *tab;
#if defined(__x86_64__)
    int level = 4;
#else
    int level = 3;
#endif

    ASSERT(cr3_ptr);
    if (mfn_invalid(cr3_ptr->mfn)) {
        warning("CR3: 0x%lx is invalid", cr3.paddr);
        return;
    }

    spin_lock(&vmap_lock);

    tab = tmp_map_mfn(cr3_ptr->mfn);
#if defined(__x86_64__)
    pml4_t *l4e = l4_table_entry((pml4_t *) tab, va);
    dump_pte(l4e, cr3_ptr->mfn, level--, l4_table_index(va));

    if (mfn_invalid(l4e->mfn))
        goto unlock;

    tab_paddr = mfn_to_paddr(l4e->mfn);
    tab = tmp_map_mfn(l4e->mfn);
#endif
    pdpe_t *l3e = l3_table_entry((pdpe_t *) tab, va);
    dump_pte(l3e, tab_paddr, level--, l3_table_index(va));

    if (mfn_invalid(l3e->mfn) || l3e->PS)
        goto unlock;

    tab_paddr = mfn_to_paddr(l3e->mfn);
    tab = tmp_map_mfn(l3e->mfn);
    pde_t *l2e = l2_table_entry((pde_t *) tab, va);
    dump_pte(l2e, tab_paddr, level--, l2_table_index(va));

    if (mfn_invalid(l2e->mfn) || l2e->PS)
        goto unlock;

    tab_paddr = mfn_to_paddr(l2e->mfn);
    tab = tmp_map_mfn(l2e->mfn);
    pte_t *l1e = l1_table_entry((pte_t *) tab, va);
    dump_pte(l1e, tab_paddr, level--, l1_table_index(va));

unlock:
    spin_unlock(&vmap_lock);
}

void dump_kern_pagetable_va(void *va) {
    dump_pagetable_va(&cr3, va);
}

void dump_user_pagetable_va(void *va) {
    dump_pagetable_va(&user_cr3, va);
}

static mfn_t get_cr3_mfn(cr3_t *cr3_entry) {
    void *cr3_mapped = NULL;

    if (mfn_invalid(cr3_entry->mfn)) {
        frame_t *frame = get_free_frame();
        BUG_ON(!frame);

        cr3_entry->mfn = frame->mfn;
        cr3_mapped = tmp_map_mfn(cr3_entry->mfn);
        memset(cr3_mapped, 0, PAGE_SIZE);
    }

    return cr3_entry->mfn;
}

static inline void pgentry_fixup_flags(pgentry_t *entry, unsigned long flags) {
    /* Our new flags may take precedence over previous ones if the new ones are more
     * permissive. */
    unsigned long entry_new = *entry;
    entry_new |= flags & _PAGE_USER; /* USER may result in crash if we enabled SMAP */
    entry_new |= flags & _PAGE_RW;
    entry_new &= ~(flags & _PAGE_NX);
    if (unlikely(*entry != entry_new)) {
        char flags_str_old[16];
        char flags_str_new[16];
        warning("Already-present PTE protection flags conflict with our.\n"
                "        Updating present flags: %s -> %s",
                dump_pte_flags(flags_str_old, 16, (pte_t) *entry),
                dump_pte_flags(flags_str_new, 16, (pte_t) entry_new));
        *entry = entry_new;
        barrier();
        flush_tlb();
    }
}

static mfn_t get_pgentry_mfn(mfn_t tab_mfn, pt_index_t index, unsigned long flags) {
    pgentry_t *tab, *entry;
    mfn_t mfn;

    BUG_ON(mfn_invalid(tab_mfn));

    tab = tmp_map_mfn(tab_mfn);
    entry = &tab[index];

    mfn = mfn_from_pgentry(*entry);
    if (mfn_invalid(mfn)) {
        frame_t *frame = get_free_frame();
        BUG_ON(!frame);

        mfn = frame->mfn;
        set_pgentry(entry, mfn, flags);
        tab = tmp_map_mfn(mfn);
        memset(tab, 0, PAGE_SIZE);
    }
    else {
        /* Page table already exists but its flags may conflict with our. Maybe fixup */
        pgentry_fixup_flags(entry, flags);
    }

    return mfn;
}

static void *_vmap(cr3_t *cr3_ptr, void *va, mfn_t mfn, unsigned int order,
#if defined(__x86_64__)
                   unsigned long l4_flags,
#endif
                   unsigned long l3_flags, unsigned long l2_flags,
                   unsigned long l1_flags) {
    mfn_t l1t_mfn, l2t_mfn, l3t_mfn;
    pgentry_t *tab, *entry;

    if (!va || (_ul(va) & ~PAGE_ORDER_TO_MASK(order)) || !is_canon_va(va))
        return NULL;

    dprintk("%s: va: 0x%p mfn: 0x%lx (order: %u)\n", __func__, va, mfn, order);

#if defined(__x86_64__)
    l3t_mfn = get_pgentry_mfn(get_cr3_mfn(cr3_ptr), l4_table_index(va), l4_flags);
#else
    l3t_mfn = get_cr3_mfn(cr3_ptr);
#endif

    if (order == PAGE_ORDER_1G) {
        tab = tmp_map_mfn(l3t_mfn);
        entry = &tab[l3_table_index(va)];
        set_pgentry(entry, mfn, l3_flags | _PAGE_PSE);
        invlpg(va);
        goto done;
    }

    l2t_mfn = get_pgentry_mfn(l3t_mfn, l3_table_index(va), l3_flags);

    if (order == PAGE_ORDER_2M) {
        tab = tmp_map_mfn(l2t_mfn);
        entry = &tab[l2_table_index(va)];
        set_pgentry(entry, mfn, l2_flags | _PAGE_PSE);
        invlpg(va);
        goto done;
    }

    l1t_mfn = get_pgentry_mfn(l2t_mfn, l2_table_index(va), l2_flags);

    tab = tmp_map_mfn(l1t_mfn);
    entry = &tab[l1_table_index(va)];
    set_pgentry(entry, mfn, l1_flags);
    invlpg(va);

done:
    return va;
}

void *vmap_kern(void *va, mfn_t mfn, unsigned int order,
#if defined(__x86_64__)
                unsigned long l4_flags,
#endif
                unsigned long l3_flags, unsigned long l2_flags, unsigned long l1_flags) {
    unsigned long _va = _ul(va) & PAGE_ORDER_TO_MASK(order);

    spin_lock(&vmap_lock);
    va = _vmap(&cr3, _ptr(_va), mfn, order, l4_flags, l3_flags, l2_flags, l1_flags);
    spin_unlock(&vmap_lock);
    return va;
}

void *vmap_user(void *va, mfn_t mfn, unsigned int order,
#if defined(__x86_64__)
                unsigned long l4_flags,
#endif
                unsigned long l3_flags, unsigned long l2_flags, unsigned long l1_flags) {
    unsigned long _va = _ul(va) & PAGE_ORDER_TO_MASK(order);

    spin_lock(&vmap_lock);
    va = _vmap(&user_cr3, _ptr(_va), mfn, order, l4_flags, l3_flags, l2_flags, l1_flags);
    spin_unlock(&vmap_lock);
    return va;
}

static inline void init_tmp_mapping(void) {
    pte_t *tab = get_l1_table(_tmp_mapping);
    _tmp_mapping_entry = (pgentry_t *) l1_table_entry(tab, _tmp_mapping);
    BUG_ON(!_tmp_mapping_entry);
}

static void map_tmp_mapping_entry(void) {
    pml4_t *l3e = l4_table_entry(mfn_to_virt(cr3.mfn), _tmp_mapping);
    pdpe_t *l2e = l3_table_entry(mfn_to_virt(l3e->mfn), _tmp_mapping);
    pde_t *l1e = l2_table_entry(mfn_to_virt(l2e->mfn), _tmp_mapping);
    pte_t *entry = l1_table_entry(mfn_to_virt(l1e->mfn), _tmp_mapping);

    /* Map _tmp_mapping_entry PTE of new page tables */
    kmap_4k(l1e->mfn, L1_PROT);

    /* Point _tmp_mapping_entry at new page tables location */
    _tmp_mapping_entry = paddr_to_virt_kern(_paddr(entry));
}

void init_pagetables(void) {
    init_tmp_mapping();

    for_each_memory_range (r) {
        switch (r->base) {
        case VIRT_IDENT_BASE:
            for (mfn_t mfn = virt_to_mfn(r->start); mfn < virt_to_mfn(r->end); mfn++)
                vmap_4k(mfn_to_virt(mfn), mfn, r->flags);
            break;
        case VIRT_KERNEL_BASE:
            for (mfn_t mfn = virt_to_mfn(r->start); mfn < virt_to_mfn(r->end); mfn++)
                kmap_4k(mfn, r->flags);
            break;
        case VIRT_USER_BASE:
            for (mfn_t mfn = virt_to_mfn(r->start); mfn < virt_to_mfn(r->end); mfn++) {
                void *va = mfn_to_virt_user(mfn);

                vmap_4k(va, mfn, r->flags);
                vmap_user_4k(va, mfn, r->flags);
            }
            break;
        default:
            break;
        }
    }

    map_frames_array();
    map_multiboot_areas();
    map_tmp_mapping_entry();

    write_cr3(cr3.paddr);
}
