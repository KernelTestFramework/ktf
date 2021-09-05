/*
 * Copyright © 2021 Amazon.com, Inc. or its affiliates.
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
#include <console.h>
#include <multiboot.h>

#include <mm/regions.h>

unsigned regions_num;

#define _RANGE(_name, _base, _flags, _start, _end)                                       \
    {                                                                                    \
        .name = _name, .base = (_base), .flags = (_flags), .start = _ptr(_start),        \
        .end = _ptr(_end)                                                                \
    }

#define IDENT_RANGE(name, flags, start, end)                                             \
    _RANGE(name, VIRT_IDENT_BASE, flags, start, end)

#define USER_RANGE(name, flags, start, end)                                              \
    _RANGE(name, VIRT_USER_BASE, flags, start, end)

#define KERNEL_RANGE(name, flags, start, end)                                            \
    _RANGE(name, VIRT_KERNEL_BASE, flags, start, end)

addr_range_t addr_ranges[] = {
    /* clang-format off */
    IDENT_RANGE( ".text.init",  L1_PROT_RO,      __start_text_init,     __end_text_init ),
    IDENT_RANGE( ".data.init",  L1_PROT,         __start_data_init,     __end_data_init ),
    IDENT_RANGE( ".bss.init",   L1_PROT,         __start_bss_init,      __end_bss_init  ),

    IDENT_RANGE( ".text.rmode", L1_PROT_RO,      __start_text_rmode,    __end_text_rmode),
    IDENT_RANGE( ".data.rmode", L1_PROT,         __start_data_rmode,    __end_data_rmode),
    IDENT_RANGE( ".bss.rmode",  L1_PROT,         __start_bss_rmode,     __end_bss_rmode ),

    USER_RANGE( ".text.user",   L1_PROT_USER_RO, __start_text_user,     __end_text_user ),
    USER_RANGE( ".data.user",   L1_PROT_USER,    __start_data_user,     __end_data_user ),
    USER_RANGE( ".bss.user",    L1_PROT_USER,    __start_bss_user,      __end_bss_user  ),

    KERNEL_RANGE( ".text",      L1_PROT_RO,      __start_text,           __end_text      ),
    KERNEL_RANGE( ".data",      L1_PROT,         __start_data,           __end_data      ),
    KERNEL_RANGE( ".extables",  L1_PROT_RO,      __start_extables,       __end_extables  ),
    KERNEL_RANGE( ".bss",       L1_PROT,         __start_bss,            __end_bss       ),
    KERNEL_RANGE( ".rodata",    L1_PROT_RO,      __start_rodata,         __end_rodata    ),
    KERNEL_RANGE( ".symbols",   L1_PROT_RO,      __start_symbols,        __end_symbols   ),
    /* clang-format on */

    {0x0} /* NULL array terminator */
};

void display_memory_map(void) {
    printk("Memory Map:\n");

    for_each_memory_range (r) {
        printk("%11s: VA: [0x%016lx - 0x%016lx] PA: [0x%08lx - 0x%08lx]\n", r->name,
               _ul(r->start), _ul(r->end), _ul(r->start - r->base),
               _ul(r->end - r->base));
    }
}

addr_range_t get_memory_range(paddr_t pa) {
    addr_range_t r;

    memset(&r, 0, sizeof(r));
    if (mbi_get_memory_range(pa, &r) < 0)
        /* FIXME: e820_lower_memory_bound() */
        panic("Unable to get memory range for: 0x%016lx\n", pa);

    return r;
}

paddr_t get_memory_range_start(paddr_t pa) {
    addr_range_t r = get_memory_range(pa);

    return _paddr(r.start);
}

paddr_t get_memory_range_end(paddr_t pa) {
    addr_range_t r = get_memory_range(pa);

    return _paddr(r.end);
}

int get_avail_memory_range(unsigned index, addr_range_t *r) {
    return mbi_get_avail_memory_range(index, r);
}

bool has_memory_range(paddr_t pa) { return mbi_get_memory_range(pa, NULL) == 0; }

void init_regions(void) { regions_num = mbi_get_avail_memory_ranges_num(); }
