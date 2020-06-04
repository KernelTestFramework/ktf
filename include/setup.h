#ifndef SETUP_SETUP_H
#define SETUP_SETUP_H

#define BDA_ADDR_START 0x400
#define BDA_ADDR_END   0x4FF

#define BDA_COM_PORTS_ENTRY 0x400
#define EBDA_ADDR_ENTRY     0x40E

#define BIOS_ROM_ADDR_START 0xF0000

#define KERN_STACK_SIZE (5 * PAGE_SIZE)

#ifndef __ASSEMBLY__
#include <page.h>
#include <string.h>

extern io_port_t com_ports[2];

extern bool opt_debug;

extern uint8_t kernel_stack[KERN_STACK_SIZE + 2 * PAGE_SIZE];
extern uint8_t user_stack[PAGE_SIZE];

#define GET_KERN_STACK()    (&kernel_stack[KERN_STACK_SIZE])
#define GET_KERN_EX_STACK() (&kernel_stack[KERN_STACK_SIZE + PAGE_SIZE])
#define GET_KERN_EM_STACK() (&kernel_stack[KERN_STACK_SIZE + PAGE_SIZE])

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
    void *from;
    void *to;
};
typedef struct addr_range addr_range_t;

extern addr_range_t addr_ranges[];
#define for_each_memory_range(ptr)                                  \
    for (addr_range_t *ptr = &addr_ranges[0];                       \
         ptr->name != NULL || (ptr->from != 0x0 && ptr->to != 0x0); \
         ptr++)

extern const char *kernel_cmdline;
extern void display_memory_map(void);
extern void dump_pagetables(void);

static inline void get_com_ports(void) {
    memcpy((void *) com_ports, (void *)(BDA_COM_PORTS_ENTRY), sizeof(com_ports));

    if (com_ports[0] == 0x0)
        com_ports[0] = 0x3f8;

    if (com_ports[1] == 0x0)
        com_ports[1] = 0x2f8;
}

#endif /* __ASSEMBLY__ */

#endif /* SETUP_SETUP_H */
