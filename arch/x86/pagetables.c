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

    if (level == 0)
        return;

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
    ASSERT(cr3_ptr);
    if (mfn_invalid(cr3_ptr->mfn)) {
        warning("CR3: 0x%lx is invalid", cr3.paddr);
        return;
    }

    printk("Page Tables: CR3 paddr: 0x%lx\n", cr3.paddr);
    spin_lock(&vmap_lock);
    dump_pagetable(cr3_ptr->mfn, PT_LEVELS);
    spin_unlock(&vmap_lock);
}

static void dump_pagetable_va(cr3_t *cr3_ptr, void *va) {
    paddr_t tab_paddr;
    pgentry_t *tab;
    int level = PT_LEVELS;

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

static inline void clean_pagetable(void *tab) {
    for (pgentry_t *e = tab; e < (pgentry_t *) (tab + PAGE_SIZE); e++)
        set_pgentry(e, MFN_INVALID, PT_NO_FLAGS);
}

static mfn_t get_cr3_mfn(cr3_t *cr3_entry) {
    void *cr3_mapped = NULL;

    if (mfn_invalid(cr3_entry->mfn)) {
        frame_t *frame = get_free_frame();
        BUG_ON(!frame);
        frame->flags.pagetable = 1;

        cr3_entry->mfn = frame->mfn;
        cr3_mapped = tmp_map_mfn(cr3_entry->mfn);
        clean_pagetable(cr3_mapped);
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
        frame->flags.pagetable = 1;

        mfn = frame->mfn;
        set_pgentry(entry, mfn, flags);
        tab = tmp_map_mfn(mfn);
        clean_pagetable(tab);
    }
    else {
        /* Page table already exists but its flags may conflict with our. Maybe fixup */
        pgentry_fixup_flags(entry, flags);
    }

    return mfn;
}

/* This function returns NULL when failed to map a non-NULL virtual address,
 * MAP_FAILED when failed to map a NULL (0x0) virtual address and otherwise
 * it returns the same virtual address passed as argument.
 */
static void *_vmap(cr3_t *cr3_ptr, void *va, mfn_t mfn, unsigned int order,
#if defined(__x86_64__)
                   unsigned long l4_flags,
#endif
                   unsigned long l3_flags, unsigned long l2_flags,
                   unsigned long l1_flags) {
    mfn_t l1t_mfn, l2t_mfn, l3t_mfn;
    pgentry_t *tab, *entry;

    if ((_ul(va) & ~PAGE_ORDER_TO_MASK(order)) || !is_canon_va(va))
        return va ? NULL : MAP_FAILED;

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

static inline void *__vmap_1g(cr3_t *cr3_ptr, void *va, mfn_t mfn, unsigned long l4_flags,
                              unsigned long l3_flags) {
    return _vmap(cr3_ptr, va, mfn, PAGE_ORDER_1G, l4_flags, l3_flags | _PAGE_PSE,
                 PT_NO_FLAGS, PT_NO_FLAGS);
}

static inline void *__vmap_2m(cr3_t *cr3_ptr, void *va, mfn_t mfn, unsigned long l4_flags,
                              unsigned long l3_flags, unsigned long l2_flags) {
    return _vmap(cr3_ptr, va, mfn, PAGE_ORDER_2M, l4_flags, l3_flags,
                 l2_flags | _PAGE_PSE, PT_NO_FLAGS);
}

static inline void *__vmap_4k(cr3_t *cr3_ptr, void *va, mfn_t mfn, unsigned long l4_flags,
                              unsigned long l3_flags, unsigned long l2_flags,
                              unsigned long l1_flags) {
    return _vmap(cr3_ptr, va, mfn, PAGE_ORDER_4K, l4_flags, l3_flags, l2_flags, l1_flags);
}

static inline void *_vmap_1g(cr3_t *cr3_ptr, void *va, mfn_t mfn, unsigned long l3_flags,
                             bool propagate_user) {
    unsigned long l4_flags = L4_PROT;

    if (propagate_user)
        l4_flags |= l3_flags & _PAGE_USER;
    return __vmap_1g(cr3_ptr, va, mfn, l4_flags, l3_flags);
}

static inline void *_vmap_2m(cr3_t *cr3_ptr, void *va, mfn_t mfn, unsigned long l2_flags,
                             bool propagate_user) {
    unsigned long l4_flags = L4_PROT;
    unsigned long l3_flags = L3_PROT;

    if (propagate_user) {
        unsigned long user_bit = l2_flags & _PAGE_USER;
        l4_flags |= user_bit;
        l3_flags |= user_bit;
    }
    return __vmap_2m(cr3_ptr, va, mfn, l4_flags, l3_flags, l2_flags);
}

static inline void *_vmap_4k(cr3_t *cr3_ptr, void *va, mfn_t mfn, unsigned long l1_flags,
                             bool propagate_user) {
    unsigned long l4_flags = L4_PROT;
    unsigned long l3_flags = L3_PROT;
    unsigned long l2_flags = L2_PROT;

    if (propagate_user) {
        unsigned long user_bit = l1_flags & _PAGE_USER;
        l4_flags |= user_bit;
        l3_flags |= user_bit;
        l2_flags |= user_bit;
    }

    return __vmap_4k(cr3_ptr, va, mfn, l4_flags, l3_flags, l2_flags, l1_flags);
}

void *vmap(cr3_t *cr3_ptr, void *va, mfn_t mfn, unsigned int order,
#if defined(__x86_64__)
           unsigned long l4_flags,
#endif
           unsigned long l3_flags, unsigned long l2_flags, unsigned long l1_flags) {
    dprintk("%s: va: 0x%p mfn: 0x%lx (order: %u)\n", __func__, va, mfn, order);

    spin_lock(&vmap_lock);
    va = _vmap(cr3_ptr, va, mfn, order, l4_flags, l3_flags, l2_flags, l1_flags);
    spin_unlock(&vmap_lock);

    return va;
}

void *vmap_1g(cr3_t *cr3_ptr, void *va, mfn_t mfn, unsigned long l3_flags,
              bool propagate_user) {
    unsigned long _va = _ul(va) & PAGE_ORDER_TO_MASK(PAGE_ORDER_1G);

    dprintk("%s: va: 0x%p mfn: 0x%lx\n", __func__, va, mfn);

    spin_lock(&vmap_lock);
    va = _vmap_1g(cr3_ptr, _ptr(_va), mfn, l3_flags, propagate_user);
    spin_unlock(&vmap_lock);

    return va;
}

void *vmap_2m(cr3_t *cr3_ptr, void *va, mfn_t mfn, unsigned long l2_flags,
              bool propagate_user) {
    unsigned long _va = _ul(va) & PAGE_ORDER_TO_MASK(PAGE_ORDER_2M);

    dprintk("%s: va: 0x%p mfn: 0x%lx\n", __func__, va, mfn);

    spin_lock(&vmap_lock);
    va = _vmap_2m(cr3_ptr, _ptr(_va), mfn, l2_flags, propagate_user);
    spin_unlock(&vmap_lock);

    return va;
}

void *vmap_4k(cr3_t *cr3_ptr, void *va, mfn_t mfn, unsigned long l1_flags,
              bool propagate_user) {
    unsigned long _va = _ul(va) & PAGE_ORDER_TO_MASK(PAGE_ORDER_4K);

    dprintk("%s: va: 0x%p mfn: 0x%lx\n", __func__, va, mfn);

    spin_lock(&vmap_lock);
    va = _vmap_4k(cr3_ptr, _ptr(_va), mfn, l1_flags, propagate_user);
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
    vmap_kern_4k(mfn_to_virt_kern(l1e->mfn), l1e->mfn, L1_PROT_GLOB);

    /* Point _tmp_mapping_entry at new page tables location */
    _tmp_mapping_entry = paddr_to_virt_kern(_paddr(entry));
}

static int _vunmap(cr3_t *cr3_ptr, void *va, mfn_t *mfn, unsigned int *order) {
    pgentry_t *tab;
    mfn_t _mfn;
    unsigned int _order;
    pgentry_t *entry;
    bool present;

    if (mfn_invalid(cr3_ptr->mfn))
        return -EINVAL;

    tab = tmp_map_mfn(cr3_ptr->mfn);
#if defined(__x86_64__)
    pml4_t *l4e = l4_table_entry((pml4_t *) tab, va);
    if (mfn_invalid(l4e->mfn) || !l4e->P)
        return -ENOENT;

    tab = tmp_map_mfn(l4e->mfn);
#endif
    pdpe_t *l3e = l3_table_entry((pdpe_t *) tab, va);
    if (l3e->PS) {
        _mfn = l3e->mfn;
        _order = PAGE_ORDER_1G;
        entry = &l3e->entry;
        present = l3e->P;
        goto done;
    }

    if (mfn_invalid(l3e->mfn) || !l3e->P)
        return -ENOENT;

    tab = tmp_map_mfn(l3e->mfn);
    pde_t *l2e = l2_table_entry((pde_t *) tab, va);
    if (l2e->PS) {
        _mfn = l2e->mfn;
        _order = PAGE_ORDER_2M;
        entry = &l2e->entry;
        present = l2e->P;
        goto done;
    }

    if (mfn_invalid(l2e->mfn) || !l2e->P)
        return -ENOENT;

    tab = tmp_map_mfn(l2e->mfn);
    pte_t *l1e = l1_table_entry((pte_t *) tab, va);
    _mfn = l1e->mfn;
    _order = PAGE_ORDER_4K;
    entry = &l1e->entry;
    present = l1e->P;

done:
    if (mfn)
        *mfn = _mfn;
    if (order)
        *order = _order;
    set_pgentry(entry, MFN_INVALID, PT_NO_FLAGS);
    if (present)
        invlpg(va);

    return 0;
}

int vunmap_kern(void *va, mfn_t *mfn, unsigned int *order) {
    int err;

    dprintk("%s: va: 0x%p (cr3: 0x%p)\n", __func__, va, &cr3);
    spin_lock(&vmap_lock);
    err = _vunmap(&cr3, va, mfn, order);
    spin_unlock(&vmap_lock);
    return err;
}

int vunmap_user(void *va, mfn_t *mfn, unsigned int *order) {
    int err;

    dprintk("%s: va: 0x%p (cr3: 0x%p)\n", __func__, va, &cr3);
    spin_lock(&vmap_lock);
    err = _vunmap(&user_cr3, va, mfn, order);
    spin_unlock(&vmap_lock);
    return err;
}

static int get_va_mfn_order(const cr3_t *cr3_ptr, const void *va, mfn_t *mfn,
                            unsigned int *order) {
    unsigned int _order;
    mfn_t _mfn;
    pgentry_t *tab;

    ASSERT(mfn || order);
    if (mfn_invalid(cr3_ptr->mfn))
        return -EINVAL;

    tab = tmp_map_mfn(cr3_ptr->mfn);
#if defined(__x86_64__)
    pml4_t *l4e = l4_table_entry((pml4_t *) tab, va);
    if (mfn_invalid(l4e->mfn) || !l4e->P)
        return -ENOENT;

    tab = tmp_map_mfn(l4e->mfn);
#endif
    pdpe_t *l3e = l3_table_entry((pdpe_t *) tab, va);
    if (mfn_invalid(l3e->mfn) || !l3e->P)
        return -ENOENT;

    if (l3e->PS) {
        _mfn = l3e->mfn;
        _order = PAGE_ORDER_1G;
        goto done;
    }

    tab = tmp_map_mfn(l3e->mfn);
    pde_t *l2e = l2_table_entry((pde_t *) tab, va);
    if (mfn_invalid(l2e->mfn) || !l2e->P)
        return -ENOENT;

    if (l2e->PS) {
        _mfn = l2e->mfn;
        _order = PAGE_ORDER_2M;
        goto done;
    }

    tab = tmp_map_mfn(l2e->mfn);
    pte_t *l1e = l1_table_entry((pte_t *) tab, va);
    if (mfn_invalid(l1e->mfn) || !l1e->P)
        return -ENOENT;

    _mfn = l1e->mfn;
    _order = PAGE_ORDER_4K;

done:
    if (mfn)
        *mfn = _mfn;
    if (order)
        *order = _order;

    return 0;
}

int get_kern_va_mfn_order(void *va, mfn_t *mfn, unsigned int *order) {
    int err;

    dprintk("%s: va: 0x%p (cr3: 0x%p)\n", __func__, va, &cr3);

    spin_lock(&vmap_lock);
    err = get_va_mfn_order(&cr3, va, mfn, order);
    spin_unlock(&vmap_lock);

    return err;
}

int get_user_va_mfn_order(void *va, mfn_t *mfn, unsigned int *order) {
    int err;

    dprintk("%s: va: 0x%p (cr3: 0x%p)\n", __func__, va, &user_cr3);

    spin_lock(&vmap_lock);
    err = get_va_mfn_order(&user_cr3, va, mfn, order);
    spin_unlock(&vmap_lock);

    return err;
}

static frame_t *find_va_frame(const cr3_t *cr3_ptr, const void *va) {
    unsigned int order;
    mfn_t mfn;
    int err;

    spin_lock(&vmap_lock);
    err = get_va_mfn_order(cr3_ptr, va, &mfn, &order);
    spin_unlock(&vmap_lock);

    return err ? NULL : find_mfn_frame(mfn, order);
}

frame_t *find_kern_va_frame(const void *va) {
    return find_va_frame(&cr3, va);
}

frame_t *find_user_va_frame(const void *va) {
    return find_va_frame(&user_cr3, va);
}

static inline void *_vmap_range_chunk(cr3_t *cr3_ptr, void *va, mfn_t mfn,
                                      unsigned int order, unsigned long flags,
                                      bool propagate_user) {
    switch (order) {
    case PAGE_ORDER_4K:
        return _vmap_4k(cr3_ptr, va, mfn, flags, propagate_user);
    case PAGE_ORDER_2M:
        return _vmap_2m(cr3_ptr, va, mfn, flags, propagate_user);
    case PAGE_ORDER_1G:
        return _vmap_1g(cr3_ptr, va, mfn, flags, propagate_user);
    default:
        BUG();
    }

    return NULL;
}

static inline int _vmap_range(mfn_t mfn, unsigned int order, unsigned long flags,
                              vmap_flags_t vmap_flags) {
    const int err = -EFAULT;

    /* NOTE: It might make sense to unmap partial completed mappings in case of an
     *       error. For now, we just return an error and let the caller handle it.
     */
    if (vmap_flags & VMAP_KERNEL) {
        if (!_vmap_range_chunk(&cr3, mfn_to_virt_kern(mfn), mfn, order, flags, false))
            return err;
    }

    if (vmap_flags & VMAP_IDENT) {
        if (virt_invalid(
                _vmap_range_chunk(&cr3, mfn_to_virt(mfn), mfn, order, flags, false)))
            return err;
    }

    if (vmap_flags & VMAP_KERNEL_MAP) {
        if (!_vmap_range_chunk(&cr3, mfn_to_virt_map(mfn), mfn, order, flags, false))
            return err;
    }

    if (vmap_flags & (VMAP_KERNEL_USER | VMAP_KERNEL_USER_ACCESS)) {
        unsigned long _flags = flags & ~_PAGE_USER;
        if (vmap_flags & VMAP_KERNEL_USER_ACCESS)
            _flags |= _PAGE_USER;

        if (!_vmap_range_chunk(&cr3, mfn_to_virt_user(mfn), mfn, order, _flags, false))
            return err;
    }

    if (vmap_flags & VMAP_USER) {
        if (!_vmap_range_chunk(&user_cr3, mfn_to_virt_user(mfn), mfn, order,
                               flags | _PAGE_USER, true))
            return err;
    }

    if (vmap_flags & VMAP_USER_IDENT) {
        if (!_vmap_range_chunk(&user_cr3, mfn_to_virt(mfn), mfn, order,
                               flags & ~_PAGE_USER, false))
            return err;
    }

    if (vmap_flags & VMAP_USER_KERNEL) {
        if (!_vmap_range_chunk(&user_cr3, mfn_to_virt_kern(mfn), mfn, order,
                               flags & ~_PAGE_USER, false))
            return err;
    }

    if (vmap_flags & VMAP_USER_KERNEL_MAP) {
        if (!_vmap_range_chunk(&user_cr3, mfn_to_virt_map(mfn), mfn, order,
                               flags & ~_PAGE_USER, false))
            return err;
    }

    return 0;
}

int vmap_range(paddr_t paddr, size_t size, unsigned long flags, vmap_flags_t vmap_flags) {
    paddr_t cur = paddr;
    paddr_t end = cur + size;
    mfn_t mfn;
    int err;

    dprintk("%s: paddr: 0x%lx, size: %lx\n", __func__, paddr, size);

    if (!has_vmap_flags(vmap_flags))
        return -EINVAL;

    /* Round up to the next page boundary unless it is page aligned */
    end = paddr_round_up(end);
    if (end <= cur)
        return -EINVAL;

    spin_lock(&vmap_lock);
    while (cur < end) {
        mfn = paddr_to_mfn(cur);

        if (size >= PAGE_SIZE_1G && !(cur % PAGE_SIZE_1G)) {
            err = _vmap_range(mfn, PAGE_ORDER_1G, flags, vmap_flags);
            if (err < 0)
                goto unlock;

            cur += PAGE_SIZE_1G;
            size -= PAGE_SIZE_1G;
            continue;
        }

        if (size >= PAGE_SIZE_2M && !(cur % PAGE_SIZE_2M)) {
            err = _vmap_range(mfn, PAGE_ORDER_2M, flags, vmap_flags);
            if (err < 0)
                goto unlock;

            cur += PAGE_SIZE_2M;
            size -= PAGE_SIZE_2M;
            continue;
        }

        err = _vmap_range(mfn, PAGE_ORDER_4K, flags, vmap_flags);
        if (err < 0)
            goto unlock;

        cur += PAGE_SIZE;
        size -= PAGE_SIZE;
    }

    BUG_ON(paddr_to_mfn(cur) != paddr_to_mfn(end));
    err = 0;

unlock:
    spin_unlock(&vmap_lock);
    return err;
}

static inline int _vunmap_range_chunk(cr3_t *cr3_ptr, void *va, unsigned int *order) {
    mfn_t mfn = MFN_INVALID;
    int err;

    err = _vunmap(cr3_ptr, va, &mfn, order);
    if (err)
        return err;

    if (mfn_invalid(mfn))
        return -ENOENT;

    BUG_ON(*order != PAGE_ORDER_4K && *order != PAGE_ORDER_2M && *order != PAGE_ORDER_1G);
    return 0;
}

static inline int _vunmap_range(cr3_t *cr3_ptr, void *start, void *end) {
    unsigned int order = PAGE_ORDER_4K;

    for (void *cur = start; cur < end; cur += ORDER_TO_SIZE(order)) {
        int err = _vunmap_range_chunk(cr3_ptr, cur, &order);
        if (err)
            return err;
    }

    return 0;
}

int vunmap_range(paddr_t paddr, size_t size, vmap_flags_t vmap_flags) {
    paddr_t start = paddr;
    paddr_t end = start + size;
    int err;

    dprintk("%s: paddr: 0x%lx, size: %lx\n", __func__, paddr, size);

    if (!has_vmap_flags(vmap_flags))
        return -EINVAL;

    /* Round up to the next page boundary unless it is page aligned */
    end = paddr_round_up(end);
    if (end <= start)
        return -EINVAL;

    spin_lock(&vmap_lock);

    if (vmap_flags & VMAP_KERNEL) {
        err = _vunmap_range(&cr3, paddr_to_virt_kern(start), paddr_to_virt_kern(end));
        if (err < 0)
            goto unlock;
    }

    if (vmap_flags & VMAP_IDENT) {
        err = _vunmap_range(&cr3, paddr_to_virt(start), paddr_to_virt(end));
        if (err < 0)
            goto unlock;
    }

    if (vmap_flags & VMAP_KERNEL_MAP) {
        err = _vunmap_range(&cr3, paddr_to_virt_map(start), paddr_to_virt_map(end));
        if (err < 0)
            goto unlock;
    }

    if (vmap_flags & (VMAP_KERNEL_USER | VMAP_KERNEL_USER_ACCESS)) {
        err = _vunmap_range(&cr3, paddr_to_virt_user(start), paddr_to_virt_user(end));
        if (err < 0)
            goto unlock;
    }

    if (vmap_flags & VMAP_USER) {
        err =
            _vunmap_range(&user_cr3, paddr_to_virt_user(start), paddr_to_virt_user(end));
        if (err < 0)
            goto unlock;
    }

    if (vmap_flags & VMAP_USER_IDENT) {
        err = _vunmap_range(&user_cr3, paddr_to_virt(start), paddr_to_virt(end));
        if (err < 0)
            goto unlock;
    }

    if (vmap_flags & VMAP_USER_KERNEL) {
        err =
            _vunmap_range(&user_cr3, paddr_to_virt_kern(start), paddr_to_virt_kern(end));
        if (err < 0)
            goto unlock;
    }

    if (vmap_flags & VMAP_USER_KERNEL_MAP) {
        err = _vunmap_range(&user_cr3, paddr_to_virt_map(start), paddr_to_virt_map(end));
        if (err < 0)
            goto unlock;
    }

    err = 0;
unlock:
    spin_unlock(&vmap_lock);
    return err;
}

static inline void init_cr3(cr3_t *cr3_ptr) {
    memset(cr3_ptr, 0, sizeof(*cr3_ptr));
    cr3_ptr->mfn = MFN_INVALID;
}

static inline bool is_huge_page_level(int level) {
    if (level == 2)
        return true;
#if defined(__i386__)
    return false;
#else
    return level == 3;
#endif
}

static void map_pagetable(cr3_t *cr3_ptr, mfn_t table, int level) {
    void *va = mfn_to_virt_kern(table);
    pte_t *pt;

    pt = _vmap(cr3_ptr, va, table, PAGE_ORDER_4K, L4_PROT, L3_PROT, L2_PROT, L1_PROT);
    BUG_ON(!pt);

    for (int i = 0; i < level_to_entries(level) && level > 1; i++) {
        if (mfn_invalid(pt[i].mfn))
            continue;

        /* Do not map 2MB/1GB large pages */
        if (!is_huge_page_level(level) || !is_pgentry_huge(pt[i].entry))
            map_pagetable(cr3_ptr, pt[i].mfn, level - 1);
    }
}

void map_pagetables(cr3_t *to_cr3, cr3_t *from_cr3) {
    ASSERT(to_cr3);
    if (mfn_invalid(to_cr3->mfn)) {
        warning("Target CR3: 0x%lx is invalid", to_cr3->paddr);
        return;
    }

    if (!from_cr3)
        from_cr3 = to_cr3;
    else if (mfn_invalid(from_cr3->mfn)) {
        warning("Source CR3: 0x%lx is invalid", from_cr3->paddr);
        return;
    }

    dprintk("Mapping all page tables of CR3: 0x%lx to CR3: 0x%lx\n", from_cr3->paddr,
            to_cr3->paddr);

    spin_lock(&vmap_lock);
    /* Assume PML4 is not mapped */
    map_pagetable(to_cr3, from_cr3->mfn, PT_LEVELS);
    spin_unlock(&vmap_lock);
}

static void unmap_pagetable(cr3_t *cr3_ptr, mfn_t table, int level) {
    mfn_t tmp_entry_mfn = virt_to_mfn(_tmp_mapping_entry);
    pte_t *pt = mfn_to_virt_kern(table);

    for (int i = 0; i < level_to_entries(level) && level > 1; i++) {
        if (mfn_invalid(pt[i].mfn))
            continue;

        /* Do not touch the tmp_mapping entry! */
        if (pt[i].mfn == tmp_entry_mfn)
            continue;

        /* Do not touch 2MB/1GB large pages */
        if (!is_huge_page_level(level) || !is_pgentry_huge(pt[i].entry))
            unmap_pagetable(cr3_ptr, pt[i].mfn, level - 1);
    }
    _vunmap(cr3_ptr, pt, NULL, NULL);
}

void unmap_pagetables(cr3_t *from_cr3, cr3_t *of_cr3) {
    ASSERT(from_cr3);
    if (mfn_invalid(from_cr3->mfn)) {
        warning("Target CR3: 0x%lx is invalid", from_cr3->paddr);
        return;
    }

    if (!of_cr3)
        of_cr3 = from_cr3;
    else if (mfn_invalid(of_cr3->mfn)) {
        warning("Source CR3: 0x%lx is invalid", of_cr3->paddr);
        return;
    }

    dprintk("Unmapping all page tables of CR3: 0x%lx from CR3: 0x%lx\n", of_cr3->paddr,
            from_cr3->paddr);

    spin_lock(&vmap_lock);
    /* Assume PML4 is mapped */
    unmap_pagetable(from_cr3, of_cr3->mfn, PT_LEVELS);
    spin_unlock(&vmap_lock);
}

int map_pagetables_va(cr3_t *cr3_ptr, void *va) {
    pgentry_t *tab;
    int err = -EINVAL;

    ASSERT(cr3_ptr);
    if (mfn_invalid(cr3_ptr->mfn)) {
        warning("CR3: 0x%lx is invalid", cr3_ptr->paddr);
        return err;
    }

    if (!is_canon_va(va)) {
        warning("Virtual address 0x%p is not canonical", va);
        return err;
    }

    err = -EFAULT;
    spin_lock(&vmap_lock);
    tab = _vmap(cr3_ptr, mfn_to_virt_kern(cr3_ptr->mfn), cr3_ptr->mfn, PAGE_ORDER_4K,
                L4_PROT, L3_PROT, L2_PROT, L1_PROT);
    if (!tab)
        goto unlock;

#if defined(__x86_64__)
    pml4_t *l4e = l4_table_entry((pml4_t *) tab, va);
    if (mfn_invalid(l4e->mfn)) {
        err = -ENOENT;
        goto unlock;
    }

    tab = _vmap(cr3_ptr, mfn_to_virt_kern(l4e->mfn), l4e->mfn, PAGE_ORDER_4K, L4_PROT,
                L3_PROT, L2_PROT, L1_PROT);
    if (!tab)
        goto unlock;
#endif

    pdpe_t *l3e = l3_table_entry((pdpe_t *) tab, va);
    if (mfn_invalid(l3e->mfn)) {
        err = -ENOENT;
        goto unlock;
    }

    if (l3e->PS)
        goto done;

    tab = _vmap(cr3_ptr, mfn_to_virt_kern(l3e->mfn), l3e->mfn, PAGE_ORDER_4K, L4_PROT,
                L3_PROT, L2_PROT, L1_PROT);
    if (!tab)
        goto unlock;

    pde_t *l2e = l2_table_entry((pde_t *) tab, va);
    if (mfn_invalid(l2e->mfn)) {
        err = -ENOENT;
        goto unlock;
    }

    if (l2e->PS)
        goto done;

    tab = _vmap(cr3_ptr, mfn_to_virt_kern(l2e->mfn), l2e->mfn, PAGE_ORDER_4K, L4_PROT,
                L3_PROT, L2_PROT, L1_PROT);
    if (!tab)
        goto unlock;

done:
    err = 0;
unlock:
    spin_unlock(&vmap_lock);
    return err;
}

int unmap_pagetables_va(cr3_t *cr3_ptr, void *va) {
    mfn_t tmp_entry_mfn = virt_to_mfn(_tmp_mapping_entry);
    pgentry_t *tab, *tables[PT_LEVELS] = {NULL};
    int level = 0;
    int err = -EINVAL;

    ASSERT(cr3_ptr);
    if (mfn_invalid(cr3_ptr->mfn)) {
        warning("CR3: 0x%lx is invalid", cr3_ptr->paddr);
        return err;
    }

    if (!is_canon_va(va)) {
        warning("Virtual address 0x%p is not canonical", va);
        return err;
    }

    err = -EFAULT;
    spin_lock(&vmap_lock);
    tab = _vmap(cr3_ptr, mfn_to_virt_kern(cr3_ptr->mfn), cr3_ptr->mfn, PAGE_ORDER_4K,
                L4_PROT, L3_PROT, L2_PROT, L1_PROT);
    if (!tab)
        goto cleanup;
    tables[level++] = tab;

#if defined(__x86_64__)
    pml4_t *l4e = l4_table_entry((pml4_t *) tab, va);
    if (mfn_invalid(l4e->mfn)) {
        err = -ENOENT;
        goto cleanup;
    }

    tab = _vmap(cr3_ptr, mfn_to_virt_kern(l4e->mfn), l4e->mfn, PAGE_ORDER_4K, L4_PROT,
                L3_PROT, L2_PROT, L1_PROT);
    if (!tab)
        goto cleanup;
    tables[level++] = tab;
#endif

    pdpe_t *l3e = l3_table_entry((pdpe_t *) tab, va);
    if (mfn_invalid(l3e->mfn)) {
        err = -ENOENT;
        goto cleanup;
    }

    if (l3e->PS)
        goto done;

    tab = _vmap(cr3_ptr, mfn_to_virt_kern(l3e->mfn), l3e->mfn, PAGE_ORDER_4K, L4_PROT,
                L3_PROT, L2_PROT, L1_PROT);
    if (!tab)
        goto cleanup;
    tables[level++] = tab;

    pde_t *l2e = l2_table_entry((pde_t *) tab, va);
    if (mfn_invalid(l2e->mfn)) {
        err = -ENOENT;
        goto cleanup;
    }

    if (l2e->PS)
        goto done;

    /* Do not touch the tmp_mapping entry! */
    if (l2e->mfn != tmp_entry_mfn)
        tables[level] = mfn_to_virt_kern(l2e->mfn);

done:
    err = 0;
cleanup:
    for (unsigned i = 0; i < ARRAY_SIZE(tables) && tables[i]; i++)
        _vunmap(cr3_ptr, tables[i], NULL, NULL);
    spin_unlock(&vmap_lock);
    return err;
}

void init_pagetables(void) {
    init_cr3(&cr3);
    init_cr3(&user_cr3);
    init_tmp_mapping();

    for_each_memory_range (r) {
        vmap_flags_t flags = VMAP_NONE;

        switch (r->base) {
        case VIRT_IDENT_BASE:
            flags = VMAP_IDENT;
            break;
        case VIRT_KERNEL_BASE:
            flags = VMAP_KERNEL;
            break;
        case VIRT_USER_BASE:
            flags = VMAP_USER | VMAP_KERNEL_USER;
            break;
        default:
            continue;
        }

        vmap_range(virt_to_paddr(r->start), r->end - r->start, r->flags, flags);
    }

    map_frames_array();
    map_multiboot_areas();
    map_tmp_mapping_entry();

    setup_tlb_global();
    write_cr3(cr3.paddr);
}
