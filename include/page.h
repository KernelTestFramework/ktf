#ifndef KTF_PAGE_H
#define KTF_PAGE_H

#include <compiler.h>

#define PAGE_SHIFT 12
#define PAGE_SIZE  (_U64(1) << PAGE_SHIFT)
#define PAGE_MASK  (~(PAGE_SIZE - 1))

#define _PAGE_PRESENT           0x0001
#define _PAGE_RW                0x0002
#define _PAGE_USER              0x0004
#define _PAGE_PWT               0x0008
#define _PAGE_PCD               0x0010
#define _PAGE_ACCESSED          0x0020
#define _PAGE_DIRTY             0x0040
#define _PAGE_AD                (_PAGE_ACCESSED | _PAGE_DIRTY)
#define _PAGE_PSE               0x0080
#define _PAGE_PAT               0x0080
#define _PAGE_GLOBAL            0x0100
#define _PAGE_AVAIL             0x0e00
#define _PAGE_PSE_PAT           0x1000
#define _PAGE_NX                (_U64(1) << 63)

#define _PAGE_ALL_FLAGS \
    (_PAGE_PRESENT | _PAGE_RW  | _PAGE_USER | _PAGE_PWT    | \
     _PAGE_PCD     | _PAGE_AD  | _PAGE_PAT  | _PAGE_GLOBAL | \
     _PAGE_PSE_PAT | _PAGE_NX)

#define PTE_FLAGS(...) (TOKEN_OR(_PAGE_, ##__VA_ARGS__))

#define L1_PROT         (_PAGE_PRESENT | _PAGE_RW | _PAGE_ACCESSED | _PAGE_DIRTY)
#define L1_PROT_RO      (_PAGE_PRESENT | _PAGE_ACCESSED)
#define L1_PROT_NOCACHE (L1_PROT | _PAGE_PCD)
#define L1_PROT_USER    (L1_PROT | _PAGE_USER)
#define L1_PROT_USER_RO (L1_PROT_RO | _PAGE_USER)

#define L2_PROT         (L1_PROT | _PAGE_DIRTY)
#define L2_PROT_RO      (L1_PROT_RO | _PAGE_DIRTY)
#define L2_PROT_USER    (L2_PROT | _PAGE_USER)
#define L2_PROT_USER_RO (L2_PROT_RO | _PAGE_USER)

#define L3_PROT         (L2_PROT | _PAGE_DIRTY)
#define L3_PROT_RO      (L2_PROT_RO | _PAGE_DIRTY)
#define L3_PROT_USER    (L3_PROT | _PAGE_USER)
#define L3_PROT_USER_RO (L3_PROT_RO | _PAGE_USER)

#define L4_PROT         (L3_PROT | _PAGE_DIRTY)
#define L4_PROT_RO      (L3_PROT_RO | _PAGE_DIRTY)
#define L4_PROT_USER    (L4_PROT | _PAGE_USER)
#define L4_PROT_USER_RO (L4_PROT_RO | _PAGE_USER)

#define PTE_ORDER 3
#define PTE_SIZE (_U32(1) << PTE_ORDER)

#define PT_ORDER 9
#define L1_PT_SHIFT PAGE_SHIFT
#define L2_PT_SHIFT (L1_PT_SHIFT + PT_ORDER)
#define L3_PT_SHIFT (L2_PT_SHIFT + PT_ORDER)
#define L4_PT_SHIFT (L3_PT_SHIFT + PT_ORDER)

#define L1_PT_ENTRIES (PAGE_SIZE / PTE_SIZE)
#define L2_PT_ENTRIES (PAGE_SIZE / PTE_SIZE)
#if defined (__x86_64__)
#define L3_PT_ENTRIES (PAGE_SIZE / PTE_SIZE)
#define L4_PT_ENTRIES (PAGE_SIZE / PTE_SIZE)
#elif defined(__i386__)
#define L3_PT_ENTRIES 4
#endif

#define L1_MAP_SPACE (L1_PT_ENTRIES * PAGE_SIZE)
#define L2_MAP_SPACE (L2_PT_ENTRIES * L1_MAP_SPACE)
#define L3_MAP_SPACE (L3_PT_ENTRIES * L2_MAP_SPACE)
#define L4_MAP_SPACE (L4_PT_ENTRIES * L3_MAP_SPACE)

#define PADDR_SHIFT 52
#define PADDR_SIZE (_U64(1) << PADDR_SHIFT)
#define PADDR_MASK (~(PADDR_SIZE - 1))

#define VIRT_KERNEL_BASE _U64(0xffffffff80000000)
#define VIRT_USER_BASE   _U64(0x0000000000400000)
#define VIRT_IDENT_BASE  _U64(0x0000000000000000)

#ifndef __ASSEMBLY__

typedef unsigned long paddr_t;
typedef unsigned long mfn_t;

#define _paddr(addr) ((paddr_t) _ul(addr))

#define PADDR_INVALID (0UL)
#define MFN_INVALID (0UL)

static inline bool paddr_invalid(paddr_t pa) { return pa == PADDR_INVALID; }
static inline bool mfn_invalid(mfn_t mfn) { return mfn == MFN_INVALID; }

static inline mfn_t paddr_to_mfn(paddr_t pa)  { return (mfn_t) (pa >> PAGE_SHIFT);    }
static inline paddr_t mfn_to_paddr(mfn_t mfn) { return (paddr_t) (mfn << PAGE_SHIFT); }

static inline void *_paddr_to_virt(paddr_t pa, unsigned long addr_space) {
    return _ptr(pa + addr_space);
}
static inline void *paddr_to_virt_kern(paddr_t pa) { return _paddr_to_virt(pa, VIRT_KERNEL_BASE); }
static inline void *paddr_to_virt_user(paddr_t pa) { return _paddr_to_virt(pa, VIRT_USER_BASE);   }
static inline void *paddr_to_virt(paddr_t pa)      { return _paddr_to_virt(pa, VIRT_IDENT_BASE);  }

static inline void *mfn_to_virt_kern(mfn_t mfn) { return paddr_to_virt_kern(mfn << PAGE_SHIFT); }
static inline void *mfn_to_virt_user(mfn_t mfn) { return paddr_to_virt_user(mfn << PAGE_SHIFT); }
static inline void *mfn_to_virt(mfn_t mfn)      { return paddr_to_virt(mfn << PAGE_SHIFT);      }

#define IS_ADDR_SPACE_VA(va, as) ((_ul(va) & (as)) == (as))

static inline paddr_t virt_to_paddr(const void *va) {
    paddr_t pa = (paddr_t) va;

    if (IS_ADDR_SPACE_VA(va, VIRT_KERNEL_BASE))
        return pa - VIRT_KERNEL_BASE;
    if (IS_ADDR_SPACE_VA(va, VIRT_USER_BASE))
        return pa - VIRT_USER_BASE;

    return pa - VIRT_IDENT_BASE;
}

static inline mfn_t virt_to_mfn(const void *va) {
    return paddr_to_mfn(virt_to_paddr(va));
}

#endif /* __ASSEMBLY__ */

#endif /* KTF_PAGE_H */
