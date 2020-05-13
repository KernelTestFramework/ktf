#include <asm-macros.h>
#include <page.h>

#define _PAGE_SUPERUSER (_PAGE_AD + _PAGE_RW + _PAGE_PRESENT)
#define _PADDR(table, shift) (((. - (table)) / PTE_SIZE) << (shift))

SECTION(.data, "aw", 0x1000)

GLOBAL(l1_pt_entries)
    .rept L1_PT_ENTRIES
    .long _PADDR(l1_pt_entries, L1_PT_SHIFT) + _PAGE_SUPERUSER
    .long 0
    .endr
END_OBJECT(l1_pt_entries)

GLOBAL(l2_pt_entries)
    .long l1_pt_entries + _PAGE_SUPERUSER
    .long 0
    .fill (L2_PT_ENTRIES - 1), PTE_SIZE, 0
END_OBJECT(l2_pt_entries)

GLOBAL(l3_pt_entries)
    .long l2_pt_entries + _PAGE_SUPERUSER
    .long 0
    .fill (L3_PT_ENTRIES - 1), PTE_SIZE, 0
END_OBJECT(l3_pt_entries)

GLOBAL(l4_pt_entries)
    .long l3_pt_entries + _PAGE_SUPERUSER
    .long 0
    .fill (L4_PT_ENTRIES - 1), PTE_SIZE, 0
END_OBJECT(l4_pt_entries)

