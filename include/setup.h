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
#ifndef KTF_SETUP_H
#define KTF_SETUP_H

#define COM1_PORT 0x3f8
#define COM2_PORT 0x2f8

#ifndef __ASSEMBLY__
#include <cmdline.h>
#include <page.h>
#include <string.h>

#include <mm/pmm.h>

struct boot_flags {
    uint64_t virt : 1, legacy_devs : 1, i8042 : 1, vga : 1, msi : 1, aspm : 1, rtc : 1,
        rsvd : 57;
};
typedef struct boot_flags boot_flags_t;

extern io_port_t com_ports[2];

extern const char *kernel_cmdline;
extern char cpu_identifier[49];
extern boot_flags_t boot_flags;

/* Static declarations */

static inline void get_com_ports(void) {
    memcpy((void *) com_ports, (void *) (BDA_COM_PORTS_ENTRY), sizeof(com_ports));

    if (com_ports[0] == 0x0)
        com_ports[0] = COM1_PORT;

    if (com_ports[1] == 0x0)
        com_ports[1] = COM2_PORT;
}

/* External declarations */

extern unsigned get_bsp_cpu_id(void);
extern void set_bsp_cpu_id(unsigned cpu_id);

extern void zap_boot_mappings(void);

#endif /* __ASSEMBLY__ */

#endif /* KTF_SETUP_H */
