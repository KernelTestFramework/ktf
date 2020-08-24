/*
 * Copyright © 2020 Amazon.com, Inc. or its affiliates.
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
#ifndef KTF_PMM_H
#define KTF_PMM_H

#define BDA_ADDR_START 0x400
#define BDA_ADDR_END   0x4FF

#define BDA_COM_PORTS_ENTRY 0x400
#define EBDA_ADDR_ENTRY     0x40E

#define BIOS_ROM_ADDR_START 0xF0000

#ifndef __ASSEMBLY__
#include <cmdline.h>
#include <list.h>
#include <page.h>

extern unsigned long __start_text[], __end_text[];
extern unsigned long __start_data[], __end_data[];
extern unsigned long __start_bss[], __end_bss[];
extern unsigned long __start_rodata[], __end_rodata[];

extern unsigned long __start_text_user[], __end_text_user[];
extern unsigned long __start_data_user[], __end_data_user[];
extern unsigned long __start_bss_user[], __end_bss_user[];

extern unsigned long __start_text_init[], __end_text_init[];
extern unsigned long __start_data_init[], __end_data_init[];
extern unsigned long __start_bss_init[], __end_bss_init[];

extern unsigned long __start_rmode[], __end_rmode[];

extern struct ktf_param __start_cmdline[], __end_cmdline[];

extern unsigned long __weak __start_symbols[], __end_symbols[];

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

struct frame {
    struct list_head list;
    mfn_t mfn;
    uint32_t refcount;
    uint32_t : 23, mapped : 1, order : 6, uncachable : 1, free : 1;
};
typedef struct frame frame_t;

#define for_each_order(order) for (int order = 0; order < MAX_PAGE_ORDER + 1; order++)

/* External definitions */

extern void display_memory_map(void);
extern void display_frames_count(void);

extern addr_range_t get_memory_range(paddr_t pa);
extern paddr_t get_memory_range_start(paddr_t pa);
extern paddr_t get_memory_range_end(paddr_t pa);

extern bool paddr_invalid(paddr_t pa);

extern void init_pmm(void);

extern mfn_t get_free_frames(unsigned int order);
extern void put_frame(mfn_t mfn, unsigned int order);
extern void reclaim_frame(mfn_t mfn, unsigned int order);

extern void map_used_memory(void);

/* Static definitions */

static inline bool mfn_invalid(mfn_t mfn) { return paddr_invalid(mfn_to_paddr(mfn)); }

static inline mfn_t get_free_frame(void) { return get_free_frames(PAGE_ORDER_4K); }

static inline bool in_text_section(const void *addr) {
    return (addr >= _ptr(__start_text) && addr < _ptr(__end_text)) ||
           (addr >= _ptr(__start_text_init) && addr < _ptr(__end_text_init));
}

static inline bool in_init_section(const void *addr) {
    return (addr >= _ptr(__start_text_init) && addr < _ptr(__end_text_init)) ||
           (addr >= _ptr(__start_data_init) && addr < _ptr(__end_data_init)) ||
           (addr >= _ptr(__start_bss_init) && addr < _ptr(__end_bss_init));
}

static inline bool in_user_section(const void *addr) {
    return (addr >= _ptr(__start_text_user) && addr < _ptr(__end_text_user)) ||
           (addr >= _ptr(__start_data_user) && addr < _ptr(__end_data_user)) ||
           (addr >= _ptr(__start_bss_user) && addr < _ptr(__end_bss_user));
}

static inline bool in_kernel_section(const void *addr) {
    return (addr >= _ptr(__start_text) && addr < _ptr(__end_text)) ||
           (addr >= _ptr(__start_data) && addr < _ptr(__end_data)) ||
           (addr >= _ptr(__start_bss) && addr < _ptr(__end_bss)) ||
           (addr >= _ptr(__start_rodata) && addr < _ptr(__end_rodata));
}

#endif /* __ASSEMBLY__ */

#endif /* KTF_PMM_H */
