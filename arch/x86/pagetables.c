#include <ktf.h>
#include <page.h>
#include <setup.h>
#include <pagetable.h>

#define L1_KERN_PT_CNT 3
#define L1_USER_PT_CNT 3
#define L1_IDENT_PT_CNT 1

pte_t l1_kern_pagetable[L1_KERN_PT_CNT][L1_PT_ENTRIES];
pte_t l1_user_pagetable[L1_USER_PT_CNT][L1_PT_ENTRIES];

pde_t l2_kern_pagetable[L2_PT_ENTRIES];
pde_t l2_user_pagetable[L2_PT_ENTRIES];

#if defined (__x86_64__)
pdpe_t l3_pagetable[L3_PT_ENTRIES];
pml4_t l4_pagetable[L4_PT_ENTRIES] __aligned(0x1000);
#elif defined (__i386__)
pdpe_t l3_pagetable[L3_PT_ENTRIES] __aligned(0x1000);
#endif

cr3_t cr3;

void init_pagetables(void) {
#if defined (__x86_64__)
    cr3.paddr = virt_to_paddr(l4_pagetable);

    /* L4 page tables mapping */
    set_pml4(_ptr(VIRT_KERNEL_BASE), virt_to_paddr(l3_pagetable), L4_PROT);
    set_pml4(_ptr(VIRT_IDENT_BASE), virt_to_paddr(l3_pagetable), L4_PROT);
#elif defined (__i386__)
    cr3.paddr = virt_to_paddr(l3_pagetable);
#endif

    /* L3 page tables mapping */
    set_pdpe(_ptr(VIRT_KERNEL_BASE), virt_to_paddr(l2_kern_pagetable), L3_PROT);
    set_pdpe(_ptr(VIRT_IDENT_BASE), virt_to_paddr(l2_kern_pagetable), L3_PROT);

    /* L2 page tables mapping: several L1 tables mapped */
    for (int i = 0; i < L1_KERN_PT_CNT; i++) {
        unsigned long va = VIRT_KERNEL_BASE + (L1_MAP_SPACE * i);

        set_pde(_ptr(va), virt_to_paddr(l1_kern_pagetable[i]), L2_PROT);
    }

    /* L1 page tables mapping: .init sections mapping */
    for (int i = 0; i < INIT_ADDR_RANGES_NUM; i++) {
        addr_range_t *r = &init_addr_ranges[i];

        for (mfn_t mfn = virt_to_mfn(r->from); mfn <= virt_to_mfn(r->to); mfn++)
            set_pte(mfn_to_virt_kern(mfn), mfn_to_paddr(mfn), r->flags);
    }

    /* L1 page tables mapping: kernel sections mapping */
    for (int i = 0; i < KERN_ADDR_RANGES_NUM; i++) {
        addr_range_t *r = &kern_addr_ranges[i];

        for (mfn_t mfn = virt_to_mfn(r->from); mfn <= virt_to_mfn(r->to); mfn++)
            set_pte(mfn_to_virt_kern(mfn), mfn_to_paddr(mfn), r->flags);
    }
}

void init_user_pagetables(void) {
#if defined (__x86_64__)
    set_pml4(_ptr(VIRT_USER_BASE), virt_to_paddr(l3_pagetable), L4_PROT_USER);
#endif
    set_pdpe(_ptr(VIRT_USER_BASE), virt_to_paddr(l2_user_pagetable), L3_PROT_USER);

    /* L2 page tables mapping: several L1 tables mapped */
    for (int i = 0; i < L1_USER_PT_CNT; i++) {
        unsigned long va = VIRT_USER_BASE + (L1_MAP_SPACE * i);

        set_pde(_ptr(va), virt_to_paddr(l1_user_pagetable[i]), L2_PROT_USER);
    }

    /* L1 page tables mapping: user sections mapping */
    for (int i = 0; i < USER_ADDR_RANGES_NUM; i++) {
        addr_range_t *r = &user_addr_ranges[i];

        for (mfn_t mfn = virt_to_mfn(r->from); mfn <= virt_to_mfn(r->to); mfn++)
            set_pte(mfn_to_virt_user(mfn), mfn_to_paddr(mfn), r->flags);
    }
}
