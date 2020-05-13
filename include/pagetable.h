#ifndef KTF_PAGETABLE_H
#define KTF_PAGETABLE_H

#include <compiler.h>
#include <page.h>

#ifndef __ASSEMBLY__

typedef uint64_t pgentry_t;

union pte {
    pgentry_t entry;
    struct __packed {
        unsigned int flags:12;
        unsigned long :52;
    };
    struct __packed {
        unsigned long paddr:52;
        unsigned int :12;
    };
    struct __packed {
        unsigned int P:1, RW:1, US:1, PWT:1, PCD:1, A:1, D:1, PAT:1, G:1;
        unsigned int AVL0:3;
        unsigned long mfn:40;
        unsigned int AVL1:11, NX:1;
    };
};
typedef union pte pte_t;

union pde {
    pgentry_t entry;
    struct __packed {
        unsigned int flags:12;
        unsigned long :52;
    };
    struct __packed {
        unsigned long paddr:52;
        unsigned int :12;
    };
    struct __packed {
        unsigned int P:1, RW:1, US:1, PWT:1, PCD:1, A:1, IGN0:1, Z:1, IGN1:1;
        unsigned int AVL0:3;
        unsigned long mfn:40;
        unsigned int AVL1:11, NX:1;
    };
};
typedef union pde pde_t;

union pdpe {
    pgentry_t entry;
    struct __packed {
        unsigned int flags:12;
        unsigned long :52;
    };
    struct __packed {
        unsigned long paddr:52;
        unsigned int :12;
    };
    struct __packed {
        unsigned int P:1, RW:1, US:1, PWT:1, PCD:1, A:1, IGN0:1, Z:1, MBZ:1;
        unsigned int AVL0:3;
        unsigned long mfn:40;
        unsigned int AVL1:11, NX:1;
    };
};
typedef union pdpe pdpe_t;

#if defined (__x86_64__)
union pml4 {
    pgentry_t entry;
    struct __packed {
        unsigned int flags:12;
        unsigned long :52;
    };
    struct __packed {
        unsigned long paddr:52;
        unsigned int :12;
    };
    struct __packed {
        unsigned int P:1, RW:1, US:1, PWT:1, PCD:1, A:1, IGN0:1, MBZ:2;
        unsigned int AVL0:3;
        unsigned long mfn:40;
        unsigned int AVL1:11, NX:1;
    };
};
typedef union pml4 pml4_t;
#endif

union cr3 {
    unsigned long reg;
    struct __packed {
        unsigned long paddr:52;
        unsigned int :12;
    };
    struct __packed {
        unsigned int RSVD0:3, PWT:1, PCD:1, RSVD1:7;
        unsigned long mfn:40;
        unsigned int RSVD2:12;
    };
};
typedef union cr3 cr3_t;

extern cr3_t cr3;

typedef unsigned int pt_index_t;

static inline pt_index_t l1_table_index(const void *va) { return (_ul(va) >> L1_PT_SHIFT) & (L1_PT_ENTRIES - 1); }
static inline pt_index_t l2_table_index(const void *va) { return (_ul(va) >> L2_PT_SHIFT) & (L2_PT_ENTRIES - 1); }
static inline pt_index_t l3_table_index(const void *va) { return (_ul(va) >> L3_PT_SHIFT) & (L3_PT_ENTRIES - 1); }
#if defined (__x86_64__)
static inline pt_index_t l4_table_index(const void *va) { return (_ul(va) >> L4_PT_SHIFT) & (L4_PT_ENTRIES - 1); }
#endif

#if defined (__x86_64__)
static inline pml4_t *l4_table_entry(pml4_t *tab, const void *va) { return &tab[l4_table_index(va)]; }
#endif
static inline pdpe_t *l3_table_entry(pdpe_t *tab, const void *va) { return &tab[l3_table_index(va)]; }
static inline pde_t  *l2_table_entry(pde_t *tab, const void *va)  { return &tab[l2_table_index(va)]; }
static inline pte_t  *l1_table_entry(pte_t *tab, const void *va)  { return &tab[l1_table_index(va)]; }

static inline pgentry_t pgentry_from_paddr(paddr_t pa, unsigned long flags) {
    return (pgentry_t) ((pa & ~(PADDR_MASK & PAGE_MASK)) | (flags & _PAGE_ALL_FLAGS));
}

static inline pgentry_t pgentry_from_mfn(mfn_t mfn, unsigned long flags) {
    return pgentry_from_paddr(mfn_to_paddr(mfn), flags);
}

static inline pgentry_t pgentry_from_virt(const void *va, unsigned long flags) {
    return pgentry_from_paddr(virt_to_paddr(va), flags);
}

static inline pml4_t *_get_l4_table(const cr3_t *cr3) {
    return (pml4_t *) mfn_to_virt_kern(cr3->mfn);
}

static inline pml4_t *get_l4_table(void) {
    return _get_l4_table(&cr3);
}

static inline pdpe_t *get_l3_table(const void *va) {
    pml4_t *l3_entry = l4_table_entry(get_l4_table(), va);

    return (pdpe_t *) mfn_to_virt_kern(l3_entry->mfn);
}

static inline pde_t *get_l2_table(const void *va) {
    pdpe_t *l2_entry = l3_table_entry(get_l3_table(va), va);

    return (pde_t *) mfn_to_virt_kern(l2_entry->mfn);
}

static inline pte_t *get_l1_table(const void *va) {
    pde_t *l1_entry = l2_table_entry(get_l2_table(va), va);

    return (pte_t *) mfn_to_virt_kern(l1_entry->mfn);
}

static inline pte_t *get_pte(const void *va) {
    return l1_table_entry(get_l1_table(va), va);
}

#if defined (__x86_64__)
static inline void set_pml4(const void *va, paddr_t pa, unsigned long flags) {
    pml4_t *l4e = l4_table_entry(get_l4_table(), va);

    l4e->entry = pgentry_from_paddr(pa, flags);
}
#endif

static inline void set_pdpe(const void *va, paddr_t pa, unsigned long flags) {
    pdpe_t *l3e = l3_table_entry(get_l3_table(va), va);

    l3e->entry = pgentry_from_paddr(pa, flags);
}

static inline void set_pde(const void *va, paddr_t pa, unsigned long flags) {
    pde_t *l2e = l2_table_entry(get_l2_table(va), va);

    l2e->entry = pgentry_from_paddr(pa, flags);
}

static inline void set_pte(const void *va, paddr_t pa, unsigned long flags) {
    pte_t *l1e = l1_table_entry(get_l1_table(va), va);

    l1e->entry = pgentry_from_paddr(pa, flags);
}

/* External declarations */

#if defined (__x86_64__)
extern pml4_t l4_pagetable[L4_PT_ENTRIES];
#elif defined (__i386__)
extern pdpe_t l3_pagetable[L3_PT_ENTRIES];
#endif

extern void init_pagetables(void);
#endif /* __ASSEMBLY__ */

#endif /* KTF_PAGETABLE_H */
