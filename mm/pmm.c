#include <ktf.h>
#include <lib.h>
#include <page.h>
#include <console.h>

#include <mm/pmm.h>
#include <drivers/vga.h>

#define _RANGE(_name, _base, _flags, _start, _end) {   \
    .name = _name, .base = (_base), .flags = (_flags), \
    .start = _ptr(_start ),                            \
    .end = _ptr(_end )                                 \
}

#define IDENT_RANGE(name, flags, start, end) \
    _RANGE(name, VIRT_IDENT_BASE, flags, start, end)

#define USER_RANGE(name, flags, start, end) \
    _RANGE(name, VIRT_USER_BASE, flags, start, end)

#define KERNEL_RANGE(name, flags, start, end) \
    _RANGE(name, VIRT_KERNEL_BASE, flags, start, end)

#define VIDEO_START (VIRT_KERNEL_BASE + VGA_START_ADDR)
#define VIDEO_END   (VIRT_KERNEL_BASE + VGA_END_ADDR)

addr_range_t addr_ranges[] = {
    IDENT_RANGE( "Low memory",  L1_PROT_RO,      0x0,               MB(1)           ),
    IDENT_RANGE( ".text.init",  L1_PROT_RO,      __start_text_init, __end_text_init ),
    IDENT_RANGE( ".data.init",  L1_PROT,         __start_data_init, __end_data_init ),
    IDENT_RANGE( ".bss.init",   L1_PROT,         __start_bss_init,  __end_bss_init  ),

    IDENT_RANGE( ".rmode",      L1_PROT,         __start_rmode,     __end_rmode     ),

    USER_RANGE( ".text.user",   L1_PROT_USER_RO, __start_text_user, __end_text_user ),
    USER_RANGE( ".data.user",   L1_PROT_USER,    __start_data_user, __end_data_user ),
    USER_RANGE( ".bss.user",    L1_PROT_USER,    __start_bss_user,  __end_bss_user  ),

    KERNEL_RANGE( "Video",      L1_PROT,         VIDEO_START,       VIDEO_END       ),
    KERNEL_RANGE( ".text",      L1_PROT_RO,      __start_text,      __end_text      ),
    KERNEL_RANGE( ".data",      L1_PROT,         __start_data,      __end_data      ),
    KERNEL_RANGE( ".bss",       L1_PROT,         __start_bss,       __end_bss       ),
    KERNEL_RANGE( ".rodata",    L1_PROT_RO,      __start_rodata,    __end_rodata    ),

    { 0x0 } /* NULL array terminator */
};

void display_memory_map(void) {
    printk("Memory Map:\n");

    for_each_memory_range(r) {
        printk("%11s: VA: [0x%016lx - 0x%016lx] PA: [0x%08lx - 0x%08lx]\n",
               r->name, r->start, r->end, r->start - r->base, r->end - r->base);
    }
}
