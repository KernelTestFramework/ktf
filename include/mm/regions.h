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
#ifndef KTF_REGIONS_H
#define KTF_REGIONS_H

#define BDA_ADDR_START 0x400
#define BDA_ADDR_END   0x4FF

#define BDA_COM_PORTS_ENTRY 0x400
#define EBDA_ADDR_ENTRY     0x40E

#define BIOS_ACPI_ROM_START 0xE0000
#define BIOS_ACPI_ROM_STOP  0x100000

#define BIOS_ROM_ADDR_START 0xF0000

#ifndef __ASSEMBLY__
#include <cmdline.h>
#include <extables.h>
#include <page.h>
#include <string.h>

extern unsigned char __start_text[], __end_text[];
extern unsigned char __start_data[], __end_data[];
extern unsigned char __start_bss[], __end_bss[];
extern unsigned char __start_rodata[], __end_rodata[];

extern unsigned char __start_text_user[], __end_text_user[];
extern unsigned char __start_data_user[], __end_data_user[];
extern unsigned char __start_bss_user[], __end_bss_user[];

extern unsigned char __start_text_init[], __end_text_init[];
extern unsigned char __start_data_init[], __end_data_init[];
extern unsigned char __start_bss_init[], __end_bss_init[];

extern unsigned char __start_text_rmode[], __end_text_rmode[];
extern unsigned char __start_data_rmode[], __end_data_rmode[];
extern struct extable_entry __start_extables[], __stop_extables[];
extern unsigned char __end_extables[];
extern unsigned char __start_bss_rmode[], __end_bss_rmode[];

extern struct ktf_param __start_cmdline[], __end_cmdline[];

extern unsigned char __weak __start_symbols[], __end_symbols[];

struct addr_range {
    const char *name;
    unsigned long base;
    unsigned long flags;
    void *start;
    void *end;
};
typedef struct addr_range addr_range_t;

extern addr_range_t addr_ranges[];
#define for_each_memory_range(ptr)                                                       \
    for (addr_range_t *ptr = &addr_ranges[0];                                            \
         ptr->name != NULL || (ptr->start != 0x0 && ptr->end != 0x0); ptr++)

extern unsigned regions_num;

/* External definitions */

extern void display_memory_map(void);

extern addr_range_t get_memory_range(paddr_t pa);
extern paddr_t get_memory_range_start(paddr_t pa);
extern paddr_t get_memory_range_end(paddr_t pa);

extern int get_avail_memory_range(unsigned index, addr_range_t *r);
extern bool has_memory_range(paddr_t pa);

extern void init_regions(void);

/* Static definitions */

static inline bool in_text_section(const void *addr) {
    return (addr >= _ptr(__start_text) && addr < _ptr(__end_text)) ||
           (addr >= _ptr(__start_text_init) && addr < _ptr(__end_text_init));
}

static inline bool in_init_section(const void *addr) {
    return (addr >= _ptr(__start_text_init) && addr < _ptr(__end_text_init)) ||
           (addr >= _ptr(__start_data_init) && addr < _ptr(__end_data_init)) ||
           (addr >= _ptr(__start_bss_init) && addr < _ptr(__end_bss_init));
}

static inline bool in_rmode_section(const void *addr) {
    return (addr >= _ptr(__start_text_rmode) && addr < _ptr(__end_text_rmode)) ||
           (addr >= _ptr(__start_data_rmode) && addr < _ptr(__end_data_rmode)) ||
           (addr >= _ptr(__start_bss_rmode) && addr < _ptr(__end_bss_rmode));
}

static inline bool in_user_section(const void *addr) {
    return (addr >= _ptr(__start_text_user) && addr < _ptr(__end_text_user)) ||
           (addr >= _ptr(__start_data_user) && addr < _ptr(__end_data_user)) ||
           (addr >= _ptr(__start_bss_user) && addr < _ptr(__end_bss_user));
}

static inline bool in_kernel_section(const void *addr) {
    return in_rmode_section(addr) ||
           (addr >= _ptr(__start_text) && addr < _ptr(__end_text)) ||
           (addr >= _ptr(__start_data) && addr < _ptr(__end_data)) ||
           (addr >= _ptr(__start_bss) && addr < _ptr(__end_bss)) ||
           (addr >= _ptr(__start_rodata) && addr < _ptr(__end_rodata)) ||
           (addr >= _ptr(__start_symbols) && addr < _ptr(__end_symbols));
}

static inline bool in_free_region(paddr_t pa) {
    return !in_kernel_section(paddr_to_virt_kern(pa)) &&
           !in_user_section(paddr_to_virt_user(pa)) &&
           !in_init_section(paddr_to_virt(pa));
}

static inline uint32_t get_bios_ebda_addr(void) {
    return (*(uint16_t *) paddr_to_virt_kern(EBDA_ADDR_ENTRY)) << 4;
}

static inline paddr_t get_region_free_start(void *from) {
    paddr_t start = _paddr(from);

    /* Find unused beginning of the region */
    while (!in_free_region(start))
        start += PAGE_SIZE;

    return start;
}

#endif /* __ASSEMBLY__ */

#endif /* KTF_REGIONS_H */
