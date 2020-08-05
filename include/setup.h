/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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
