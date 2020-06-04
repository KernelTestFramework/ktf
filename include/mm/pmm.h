#ifndef KTF_PMM_H
#define KTF_PMM_H

#define BDA_ADDR_START 0x400
#define BDA_ADDR_END   0x4FF

#define BDA_COM_PORTS_ENTRY 0x400
#define EBDA_ADDR_ENTRY     0x40E

#define BIOS_ROM_ADDR_START 0xF0000

#ifndef __ASSEMBLY__
#include <list.h>
#include <page.h>

extern unsigned long __start_text[], __end_text[];
extern unsigned long __start_data[], __end_data[];
extern unsigned long __start_bss[],  __end_bss[];
extern unsigned long __start_rodata[], __end_rodata[];

extern unsigned long __start_text_user[], __end_text_user[];
extern unsigned long __start_data_user[], __end_data_user[];
extern unsigned long __start_bss_user[], __end_bss_user[];

extern unsigned long __start_text_init[], __end_text_init[];
extern unsigned long __start_data_init[], __end_data_init[];
extern unsigned long __start_bss_init[], __end_bss_init[];

extern unsigned long __start_rmode[], __end_rmode[];

struct addr_range {
    const char *name;
    unsigned long base;
    unsigned long flags;
    void *start;
    void *end;
};
typedef struct addr_range addr_range_t;

extern addr_range_t addr_ranges[];
#define for_each_memory_range(ptr)                                    \
    for (addr_range_t *ptr = &addr_ranges[0];                         \
         ptr->name != NULL || (ptr->start != 0x0 && ptr->end != 0x0); \
         ptr++)

struct frame {
    struct list_head list;
    mfn_t mfn;
    uint32_t refcount;
    uint32_t :24, order:6, uncachable:1, free:1;
};
typedef struct frame frame_t;

#define for_each_order(order) \
    for (int order = 0; order < MAX_PAGE_ORDER + 1; order++)

/* External definitions */

extern void display_memory_map(void);

extern addr_range_t get_memory_range(paddr_t pa);
extern paddr_t get_memory_range_start(paddr_t pa);
extern paddr_t get_memory_range_end(paddr_t pa);

extern void init_pmm(void);

#endif /* __ASSEMBLY__ */

#endif /* KTF_PMM_H */
