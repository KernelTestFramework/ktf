#include <ktf.h>
#include <page.h>
#include <console.h>
#include <setup.h>
#include <string.h>
#include <pagetable.h>

#define L1_KERN_PT_CNT 3
#define L1_USER_PT_CNT 3
#define L1_IDENT_PT_CNT 1

pte_t l1_kern_pagetable[L1_KERN_PT_CNT][L1_PT_ENTRIES];
pte_t l1_user_pagetable[L1_USER_PT_CNT][L1_PT_ENTRIES];

pde_t l2_pagetable[L2_PT_ENTRIES];

#if defined (__x86_64__)
pdpe_t l3_pagetable_kern[L3_PT_ENTRIES];
pdpe_t l3_pagetable_ident[L3_PT_ENTRIES];
pml4_t l4_pagetable[L4_PT_ENTRIES] __aligned(0x1000);
#elif defined (__i386__)
pdpe_t l3_pagetable[L3_PT_ENTRIES] __aligned(0x1000);
#endif

cr3_t cr3;

static inline const char *dump_pte_flags(char *buf, size_t size, pgentry_t pgentry) {
    pte_t pte = (pte_t) pgentry;

    snprintf(buf, size, "%c%c%c%c%c%c%c%c%c%c",
        pte.P ? 'P' : '-',
        pte.RW ? 'W' : 'R',
        pte.US ? 'U' : 'S',
        pte.PWT ? 'w' : '-',
        pte.PCD ? '-' : 'C',
        pte.A ? 'A' : '-',
        pte.D ? 'D' : '-',
        pte.PAT ? 'p' : '-',
        pte.G ? 'G' : '-',
        pte.NX ? 'X' : '-');

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

        dump_pte_flags(flags, sizeof(flags), pt[i].entry);
        paddr_t paddr = mfn_to_paddr(pt[i].mfn);
        printk("[%p] %*s%d[%03u] paddr: %p flags: %s\n",
               virt_to_paddr(pt), (4 - level) * 2, "L", level, i, paddr, flags);

        dump_page_table(paddr_to_virt_kern(paddr), level - 1);
    }
}

void dump_pagetables(void) {
    printk("\nPage Tables:\n");
    dump_page_table(get_l4_table(), 4);
}

void init_pagetables(void) {
#if defined (__x86_64__)
    cr3.paddr = virt_to_paddr(l4_pagetable);

    /* L4 page tables mapping */
    set_pml4(_ptr(VIRT_KERNEL_BASE), virt_to_paddr(l3_pagetable_kern), L4_PROT);
    set_pml4(_ptr(VIRT_IDENT_BASE), virt_to_paddr(l3_pagetable_ident), L4_PROT);
#elif defined (__i386__)
    cr3.paddr = virt_to_paddr(l3_pagetable);
#endif

    /* L3 page tables mapping */
    set_pdpe(_ptr(VIRT_KERNEL_BASE), virt_to_paddr(l2_pagetable), L3_PROT);
    set_pdpe(_ptr(VIRT_IDENT_BASE), virt_to_paddr(l2_pagetable), L3_PROT);

    /* L2 page tables mapping: several L1 tables mapped */
    for (int i = 0; i < L1_KERN_PT_CNT; i++) {
        unsigned long va = VIRT_KERNEL_BASE + (L1_MAP_SPACE * i);

        set_pde(_ptr(va), virt_to_paddr(l1_kern_pagetable[i]), L2_PROT);
    }

    /* L1 page tables mapping: .init sections mapping */
    for_each_memory_range(r) {
        switch (r->base) {
        case VIRT_IDENT_BASE:
        case VIRT_KERNEL_BASE:
            for (mfn_t mfn = virt_to_mfn(r->from); mfn <= virt_to_mfn(r->to); mfn++)
                set_pte(mfn_to_virt_kern(mfn), mfn_to_paddr(mfn), r->flags);
            break;
        default:
            break;
        }
    }
}

void init_user_pagetables(void) {
#if defined (__x86_64__)
    set_pml4(_ptr(VIRT_USER_BASE), virt_to_paddr(l3_pagetable_ident), L4_PROT_USER);
#endif
    set_pdpe(_ptr(VIRT_USER_BASE), virt_to_paddr(l2_pagetable), L3_PROT_USER);

    /* L2 page tables mapping: several L1 tables mapped */
    for (int i = 0; i < L1_USER_PT_CNT; i++) {
        unsigned long va = VIRT_USER_BASE + (L1_MAP_SPACE * i);

        set_pde(_ptr(va), virt_to_paddr(l1_user_pagetable[i]), L2_PROT_USER);
    }

    for_each_memory_range(r) {
        switch (r->base) {
        case VIRT_USER_BASE:
            for (mfn_t mfn = virt_to_mfn(r->from); mfn <= virt_to_mfn(r->to); mfn++)
                set_pte(mfn_to_virt_user(mfn), mfn_to_paddr(mfn), r->flags);
            break;
        default:
            break;
        }
    }
}
