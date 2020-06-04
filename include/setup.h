#ifndef KTF_SETUP_H
#define KTF_SETUP_H

#define KERN_STACK_SIZE (5 * PAGE_SIZE)

#ifndef __ASSEMBLY__
#include <page.h>
#include <string.h>

#include <mm/pmm.h>

extern io_port_t com_ports[2];

extern bool opt_debug;

extern uint8_t kernel_stack[KERN_STACK_SIZE + 2 * PAGE_SIZE];
extern uint8_t user_stack[PAGE_SIZE];

#define GET_KERN_STACK()    (&kernel_stack[KERN_STACK_SIZE])
#define GET_KERN_EX_STACK() (&kernel_stack[KERN_STACK_SIZE + PAGE_SIZE])
#define GET_KERN_EM_STACK() (&kernel_stack[KERN_STACK_SIZE + 2 * PAGE_SIZE])

extern const char *kernel_cmdline;

static inline void get_com_ports(void) {
    memcpy((void *) com_ports, (void *)(BDA_COM_PORTS_ENTRY), sizeof(com_ports));

    if (com_ports[0] == 0x0)
        com_ports[0] = 0x3f8;

    if (com_ports[1] == 0x0)
        com_ports[1] = 0x2f8;
}

#endif /* __ASSEMBLY__ */

#endif /* KTF_SETUP_H */
