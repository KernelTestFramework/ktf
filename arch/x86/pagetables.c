#include <ktf.h>
#include <page.h>
#include <console.h>
#include <setup.h>
#include <string.h>
#include <pagetable.h>

cr3_t __data_init cr3;

static inline const char *dump_pte_flags(char *buf, size_t size, pte_t pte) {
    snprintf(buf, size, "%c %c%c%c%c%c%c%c%c%c",
        pte.NX  ? 'X' : '-',
        pte.G   ? 'G' : '-',
        pte.PAT ? 'p' : '-',
        pte.D   ? 'D' : '-',
        pte.A   ? 'A' : '-',
        pte.PCD ? '-' : 'C',
        pte.PWT ? 'w' : '-',
        pte.US  ? 'U' : 'S',
        pte.RW  ? 'W' : 'R',
        pte.P   ? 'P' : '-');

    return buf;
}

static inline void dump_page_table(void *table, int level) {
    char flags[16];
    int entries;
    pte_t *pt = table;

    switch (level) {
    case 4:
        entries = L4_PT_ENTRIES;
        break;
    case 3:
        entries = L3_PT_ENTRIES;
        break;
    case 2:
        entries = L2_PT_ENTRIES;
        break;
    case 1:
        entries = L1_PT_ENTRIES;
        break;
    default:
        return;
    };

    for (int i = 0; i < entries; i++) {
        if (!pt[i].P)
            continue;

        dump_pte_flags(flags, sizeof(flags), pt[i]);
        paddr_t paddr = mfn_to_paddr(pt[i].mfn);
        printk("[%p] %*s%d[%03u] paddr: 0x%016lx flags: %s\n",
               virt_to_paddr(pt), (4 - level) * 2, "L", level, i, paddr, flags);

        dump_page_table(paddr_to_virt_kern(paddr), level - 1);
    }
}

void dump_pagetables(void) {
    printk("\nPage Tables:\n");
    printk("CR3: paddr: 0x%lx\n", cr3.paddr);
    dump_page_table(get_l4_table(), 4);
}

static void *init_map_mfn(mfn_t mfn) {
    static uint8_t _tmp[PAGE_SIZE] __aligned(PAGE_SIZE);
    pgentry_t *e;

    if (mfn_invalid(mfn))
        return NULL;

    e = (pgentry_t *) l1_table_entry(get_l1_table(_tmp), _tmp);
    set_pgentry(e, mfn, L1_PROT);

    return _tmp;
}

static mfn_t get_cr3_mfn(cr3_t *cr3) {
    if (mfn_invalid(cr3->mfn))
        cr3->mfn = get_free_frame();

    return cr3->mfn;
}

static mfn_t get_pgentry_mfn(mfn_t tab_mfn, pt_index_t index, unsigned long flags) {
    pgentry_t *tab, *entry;
    mfn_t mfn;

    if (mfn_invalid(tab_mfn))
        return MFN_INVALID;

    tab = init_map_mfn(tab_mfn);
    entry = &tab[index];

    mfn = mfn_from_pgentry(*entry);
    if (mfn_invalid(mfn)) {
        set_pgentry(entry, get_free_frame(), flags);
        mfn = mfn_from_pgentry(*entry);
    }

    return mfn;
}

void *vmap(void *va, mfn_t mfn, unsigned int order, unsigned long flags) {
    mfn_t l1t_mfn, l2t_mfn, l3t_mfn;
    pgentry_t *tab, *entry;

    if (!va || _ul(va) & ~PAGE_MASK)
        return NULL;

    dprintk("%s: va: %p mfn: 0x%lx (order: %u)\n", __func__, va, mfn, order);

#if defined (__x86_64__)
    l3t_mfn = get_pgentry_mfn(get_cr3_mfn(&cr3), l4_table_index(va), L4_PROT_USER);
#else
    l3t_mfn = get_cr3_mfn(&cr3);
#endif

    if (order == PAGE_ORDER_1G) {
        tab = init_map_mfn(l3t_mfn);
        entry = &tab[l3_table_index(va)];
        set_pgentry(entry, mfn, flags | _PAGE_PSE);
        return va;
    }

    l2t_mfn = get_pgentry_mfn(l3t_mfn, l3_table_index(va), L3_PROT_USER);

    if (order == PAGE_ORDER_2M) {
        tab = init_map_mfn(l2t_mfn);
        entry = &tab[l2_table_index(va)];
        set_pgentry(entry, mfn, flags | _PAGE_PSE);
        return va;
    }

    l1t_mfn = get_pgentry_mfn(l2t_mfn, l2_table_index(va), L2_PROT_USER);

    tab = init_map_mfn(l1t_mfn);
    entry = &tab[l1_table_index(va)];
    set_pgentry(entry, mfn, flags);
    return va;
}

void *vunmap(void *va, unsigned int order) {
    return vmap(va, MFN_INVALID, order, 0x0);
}

void *kmap(mfn_t mfn, unsigned int order, unsigned long flags) {
    return vmap(mfn_to_virt_kern(mfn), mfn, order, flags);
}

void *kunmap(void *va, unsigned int order) {
    return vmap(va, MFN_INVALID, order, 0x0);
}

void init_pagetables(void) {
    for_each_memory_range(r) {
        switch (r->base) {
        case VIRT_IDENT_BASE:
            for (mfn_t mfn = virt_to_mfn(r->start); mfn < virt_to_mfn(r->end); mfn++)
                vmap(mfn_to_virt(mfn), mfn, PAGE_ORDER_4K, r->flags);
            break;
        case VIRT_KERNEL_BASE:
            for (mfn_t mfn = virt_to_mfn(r->start); mfn < virt_to_mfn(r->end); mfn++)
                kmap(mfn, PAGE_ORDER_4K, r->flags);
            break;
        case VIRT_USER_BASE:
            for (mfn_t mfn = virt_to_mfn(r->start); mfn < virt_to_mfn(r->end); mfn++)
                vmap(mfn_to_virt_user(mfn), mfn, PAGE_ORDER_4K, r->flags);
            break;
        default:
            break;
        }
    }

    map_used_memory();
}
