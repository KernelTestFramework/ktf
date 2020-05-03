#include <ktf.h>
#include <asm-macros.h>
#include <desc.h>

gdtdesc_t gdt[] __aligned(16) = {
	[GDT_NULL]      = _U64(0),
	[GDT_KERN_CS32] = GDT_ENTRY(0xcf9b, 0, 0xfffff), /* Flags: Gr=1, Sz=1; Access Byte: P=1, DPL=0, S=1, E=1, RW=1, A=1 */
	[GDT_KERN_DS32] = GDT_ENTRY(0xcf93, 0, 0xfffff), /* Flags: Gr=1, Sz=1; Access Byte: P=1, DPL=0, S=1, E=0, RW=1, A=1 */
	[GDT_KERN_CS64] = GDT_ENTRY(0xa09b, 0, 0x00000), /* Flags: Gr=1,  L=1; Access Byte: P=1, DPL=0, S=1, E=1, RW=1, A=1 */
};

gdt_ptr_t gdt_ptr __section(".data") = {
	.size = sizeof(gdt) - 1,
	.addr = _ul(&gdt[0]),
};

