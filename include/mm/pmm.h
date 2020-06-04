#ifndef KTF_PMM_H
#define KTF_PMM_H

#define BDA_ADDR_START 0x400
#define BDA_ADDR_END   0x4FF

#define BDA_COM_PORTS_ENTRY 0x400
#define EBDA_ADDR_ENTRY     0x40E

#define BIOS_ROM_ADDR_START 0xF0000

#ifndef __ASSEMBLY__
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

/* External definitions */

extern void display_memory_map(void);

#endif /* __ASSEMBLY__ */

#endif /* KTF_PMM_H */
