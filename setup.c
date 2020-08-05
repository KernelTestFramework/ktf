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
#include <ktf.h>
#include <lib.h>
#include <page.h>
#include <traps.h>
#include <string.h>
#include <setup.h>
#include <segment.h>
#include <console.h>
#include <pagetable.h>
#include <multiboot.h>
#include <apic.h>
#include <percpu.h>

#include <mm/pmm.h>
#include <mm/vmm.h>
#include <smp/smp.h>

#include <drivers/serial.h>

bool opt_debug;

io_port_t com_ports[2];

const char *kernel_cmdline;

static void init_console(void) {
    get_com_ports();

    uart_init(com_ports[0], DEFAULT_BAUD_SPEED);

    register_console_callback(serial_console_write);
    register_console_callback(vga_console_write);

    printk("COM1: %x, COM2: %x\n", com_ports[0], com_ports[1]);
}

static __always_inline void zero_bss(void) {
    memset(_ptr(__start_bss), 0x0, _ptr(__end_bss) - _ptr(__start_bss));
    memset(_ptr(__start_bss_user), 0x0, _ptr(__end_bss_user) - _ptr(__start_bss_user));
}

static __always_inline void zap_boot_mappings(void) {
#if defined (__x86_64__)
    memset(paddr_to_virt_kern(virt_to_paddr(l4_pt_entries)), 0, L4_PT_ENTRIES * sizeof(pgentry_t));
#endif
    memset(paddr_to_virt_kern(virt_to_paddr(l3_pt_entries)), 0, L3_PT_ENTRIES * sizeof(pgentry_t));
    memset(paddr_to_virt_kern(virt_to_paddr(l2_pt_entries)), 0, L2_PT_ENTRIES * sizeof(pgentry_t));
    memset(paddr_to_virt_kern(virt_to_paddr(l1_pt_entries)), 0, L1_PT_ENTRIES * sizeof(pgentry_t));
}

void __noreturn __text_init kernel_start(uint32_t multiboot_magic, multiboot_info_t *mbi) {
    /* Zero-out BSS sections */
    zero_bss();

    /* Initialize console early */
    init_console();

    if (multiboot_magic == MULTIBOOT_BOOTLOADER_MAGIC) {
        /* Indentity mapping is still on, so fill in multiboot structures */
        init_multiboot(mbi, &kernel_cmdline);
    }

    /* Initialize Physical Memory Manager */
    init_pmm();

    /* Setup final pagetables */
    init_pagetables();

    write_cr3(cr3.paddr);

    write_sp(get_free_pages_top(PAGE_ORDER_2M, GFP_KERNEL));

    if (opt_debug)
        dump_pagetables();

    /* TODO: Exception tables */

    init_percpu();

    init_traps(0);

    zap_boot_mappings();

    init_apic(APIC_MODE_XAPIC);

    /* TODO: SMP support */

    smp_init();

    /* Jump from .text.init section to .text */
    asm volatile("push %0; ret" :: "r" (&kernel_main));

    UNREACHABLE();
}
