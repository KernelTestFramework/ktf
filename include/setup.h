#ifndef KTF_SETUP_H
#define KTF_SETUP_H

#define KERN_STACK_SIZE (5 * PAGE_SIZE)

#ifndef __ASSEMBLY__
#include <page.h>
#include <string.h>

#include <mm/pmm.h>

extern io_port_t com_ports[2];

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
