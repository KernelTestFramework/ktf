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
#ifndef KTF_PAGETABLE_H
#define KTF_PAGETABLE_H

#include <compiler.h>
#include <lib.h>
#include <page.h>

#include <mm/pmm.h>

#ifndef __ASSEMBLY__

typedef uint64_t pgentry_t;

union pte {
    pgentry_t entry;
    struct __packed {
        unsigned int flags : 12;
        unsigned long : 47;
        unsigned int flags_top : 5;
    };
    struct __packed {
        unsigned long paddr : 52;
        unsigned int : 12;
    };
    struct __packed {
        unsigned int P : 1, RW : 1, US : 1, PWT : 1, PCD : 1, A : 1, D : 1, PAT : 1,
            G : 1;
        unsigned int IGN0 : 3;
        unsigned long mfn : 40;
        unsigned int IGN1 : 7;
        unsigned int PKE : 4, NX : 1;
    };
};
typedef union pte pte_t;

union pde {
    pgentry_t entry;
    struct __packed {
        unsigned int flags : 12;
        unsigned long : 51;
        unsigned int flags_top : 1;
    };
    struct __packed {
        unsigned long paddr : 52;
        unsigned int : 12;
    };
    struct __packed {
        unsigned int P : 1, RW : 1, US : 1, PWT : 1, PCD : 1, A : 1, IGN0 : 1, PS : 1;
        unsigned int IGN1 : 4;
        unsigned long mfn : 40;
        unsigned int IGN2 : 11, NX : 1;
    };
};
typedef union pde pde_t;

union pdpe {
    pgentry_t entry;
    struct __packed {
        unsigned int flags : 12;
        unsigned long : 51;
        unsigned int flags_top : 1;
    };
    struct __packed {
        unsigned long paddr : 52;
        unsigned int : 12;
    };
    struct __packed {
        unsigned int P : 1, RW : 1, US : 1, PWT : 1, PCD : 1, A : 1, IGN0 : 1, PS : 1;
        unsigned int IGN1 : 4;
        unsigned long mfn : 40;
        unsigned int IGN2 : 11, NX : 1;
    };
};
typedef union pdpe pdpe_t;

#if defined(__x86_64__)
union pml4 {
    pgentry_t entry;
    struct __packed {
        unsigned int flags : 12;
        unsigned long : 51;
        unsigned int flags_top : 1;
    };
    struct __packed {
        unsigned long paddr : 52;
        unsigned int : 12;
    };
    struct __packed {
        unsigned int P : 1, RW : 1, US : 1, PWT : 1, PCD : 1, A : 1, IGN0 : 1, Z : 1;
        unsigned int IGN1 : 4;
        unsigned long mfn : 40;
        unsigned int IGN2 : 11, NX : 1;
    };
};
typedef union pml4 pml4_t;
#endif

union cr3 {
    unsigned long reg;
    struct __packed {
        unsigned long paddr : 52;
        unsigned int : 12;
    };
    struct __packed {
        unsigned int IGN0 : 3, PWT : 1, PCD : 1, IGN1 : 7;
        unsigned long mfn : 40;
        unsigned int RSVD : 12;
    };
    struct __packed {
        unsigned int PCID : 12;
        unsigned long : 52;
    };
};
typedef union cr3 cr3_t;

extern cr3_t cr3;

typedef unsigned int pt_index_t;

static inline pt_index_t l1_table_index(const void *va) {
    return (_ul(va) >> L1_PT_SHIFT) & (L1_PT_ENTRIES - 1);
}
static inline pt_index_t l2_table_index(const void *va) {
    return (_ul(va) >> L2_PT_SHIFT) & (L2_PT_ENTRIES - 1);
}
static inline pt_index_t l3_table_index(const void *va) {
    return (_ul(va) >> L3_PT_SHIFT) & (L3_PT_ENTRIES - 1);
}
#if defined(__x86_64__)
static inline pt_index_t l4_table_index(const void *va) {
    return (_ul(va) >> L4_PT_SHIFT) & (L4_PT_ENTRIES - 1);
}
#endif

static inline unsigned long l1_index_to_virt(pt_index_t idx) {
    return _ul(idx) << L1_PT_SHIFT;
}
static inline unsigned long l2_index_to_virt(pt_index_t idx) {
    return _ul(idx) << L2_PT_SHIFT;
}
static inline unsigned long l3_index_to_virt(pt_index_t idx) {
    return _ul(idx) << L3_PT_SHIFT;
}
#if defined(__x86_64__)
static inline unsigned long l4_index_to_virt(pt_index_t idx) {
    return _ul(idx) << L4_PT_SHIFT;
}
#endif

static inline void *virt_from_index(pt_index_t l4, pt_index_t l3, pt_index_t l2,
                                    pt_index_t l1) {
    return _ptr(l4_index_to_virt(l4) | l3_index_to_virt(l3) | l2_index_to_virt(l2) |
                l1_index_to_virt(l1));
}

#if defined(__x86_64__)
static inline pml4_t *l4_table_entry(pml4_t *tab, const void *va) {
    return &tab[l4_table_index(va)];
}
#endif
static inline pdpe_t *l3_table_entry(pdpe_t *tab, const void *va) {
    return &tab[l3_table_index(va)];
}
static inline pde_t *l2_table_entry(pde_t *tab, const void *va) {
    return &tab[l2_table_index(va)];
}
static inline pte_t *l1_table_entry(pte_t *tab, const void *va) {
    return &tab[l1_table_index(va)];
}

static inline pgentry_t pgentry_from_paddr(paddr_t pa, unsigned long flags) {
    return (pgentry_t)((pa & ~(PADDR_MASK & PAGE_MASK)) | (flags & _PAGE_ALL_FLAGS));
}

static inline paddr_t paddr_from_pgentry(pgentry_t pgentry) {
    return (paddr_t)(pgentry & ~PADDR_MASK) & PAGE_MASK;
}

static inline pgentry_t pgentry_from_mfn(mfn_t mfn, unsigned long flags) {
    return pgentry_from_paddr(mfn_to_paddr(mfn), flags);
}

static inline mfn_t mfn_from_pgentry(pgentry_t pgentry) {
    return paddr_to_mfn(paddr_from_pgentry(pgentry));
}

static inline pgentry_t pgentry_from_virt(const void *va, unsigned long flags) {
    return pgentry_from_paddr(virt_to_paddr(va), flags);
}

#define INVALID_PGENTRY(e) (!(e) || mfn_invalid((e)->mfn))

#if defined(__x86_64__)
static inline pml4_t *get_l4_table(void) { return paddr_to_virt_kern(read_cr3()); }

static inline pdpe_t *get_l3_table(const void *va) {
    pml4_t *l3e = l4_table_entry(get_l4_table(), va);

    return INVALID_PGENTRY(l3e) ? NULL : mfn_to_virt_kern(l3e->mfn);
}
#elif defined(__i386__)
static inline pdpe_t *get_l3_table(void) { return paddr_to_virt_kern(read_cr3()); }
#endif

static inline pde_t *get_l2_table(const void *va) {
    pdpe_t *l2e = l3_table_entry(get_l3_table(va), va);

    return INVALID_PGENTRY(l2e) ? NULL : mfn_to_virt_kern(l2e->mfn);
}

static inline pte_t *get_l1_table(const void *va) {
    pde_t *l1e = l2_table_entry(get_l2_table(va), va);

    return INVALID_PGENTRY(l1e) ? NULL : mfn_to_virt_kern(l1e->mfn);
}

static inline void set_pgentry(pgentry_t *e, mfn_t mfn, unsigned long flags) {
    *e = pgentry_from_mfn(mfn, flags);
    barrier();
    flush_tlb();
}

/* External declarations */

extern pte_t l1_pt_entries[L1_PT_ENTRIES];
extern pde_t l2_pt_entries[L2_PT_ENTRIES];
extern pdpe_t l3_pt_entries[L3_PT_ENTRIES];
#if defined(__x86_64__)
extern pml4_t l4_pt_entries[L4_PT_ENTRIES];
#elif defined(__i386__)
#endif

extern void init_pagetables(void);
extern void dump_pagetables(void);

#endif /* __ASSEMBLY__ */

#endif /* KTF_PAGETABLE_H */
