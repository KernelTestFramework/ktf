#include <ktf.h>
#include <asm-macros.h>
#include <desc.h>

gdtdesc_t gdt[] __aligned(16) = {
    [GDT_NULL]      = GDT_ENTRY(0x0, 0x0, 0x0),
    [GDT_KERN_CS32] = GDT_ENTRY(DESC_FLAGS(GR, SZ, P, DPL0, S, CODE, R, A), 0x0, 0xfffff),
    [GDT_KERN_DS32] = GDT_ENTRY(DESC_FLAGS(GR, SZ, P, DPL0, S, DATA, W, A), 0x0, 0xfffff),
    [GDT_KERN_CS64] = GDT_ENTRY(DESC_FLAGS(GR,  L, P, DPL0, S, CODE, R, A), 0x0, 0x00000),
};

gdt_ptr_t gdt_ptr __section(".data") = {
    .size = sizeof(gdt) - 1,
    .addr = _ul(&gdt),
};

