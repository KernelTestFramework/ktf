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

#endif /* KTF_PAGE_H */
