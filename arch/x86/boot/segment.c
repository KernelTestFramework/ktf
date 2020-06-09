#include <ktf.h>
#include <asm-macros.h>
#include <segment.h>

gdt_desc_t boot_gdt[] __aligned(16) __data_init = {
    [GDT_NULL].desc      = GDT_ENTRY(0x0, 0x0, 0x0),
    [GDT_KERN_CS32].desc = GDT_ENTRY(DESC_FLAGS(GR, SZ, P, DPL0, S, CODE, R, A), 0x0, 0xfffff),
    [GDT_KERN_DS32].desc = GDT_ENTRY(DESC_FLAGS(GR, SZ, P, DPL0, S, DATA, W, A), 0x0, 0xfffff),
    [GDT_KERN_CS64].desc = GDT_ENTRY(DESC_FLAGS(GR,  L, P, DPL0, S, CODE, R, A), 0x0, 0x00000),
};

gdt_ptr_t boot_gdt_ptr __data_init = {
    .size = sizeof(boot_gdt) - 1,
    .addr = _ul(&boot_gdt),
};
