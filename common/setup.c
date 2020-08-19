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
#include <acpi.h>
#include <apic.h>
#include <console.h>
#include <ktf.h>
#include <lib.h>
#include <multiboot.h>
#include <page.h>
#include <pagetable.h>
#include <percpu.h>
#include <sched.h>
#include <segment.h>
#include <setup.h>
#include <string.h>
#include <traps.h>

#include <mm/pmm.h>
#include <mm/vmm.h>
#include <smp/smp.h>

#include <drivers/serial.h>
#include <slab.h>

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
#if defined(__x86_64__)
    memset(paddr_to_virt_kern(virt_to_paddr(l4_pt_entries)), 0,
           L4_PT_ENTRIES * sizeof(pgentry_t));
#endif
    memset(paddr_to_virt_kern(virt_to_paddr(l3_pt_entries)), 0,
           L3_PT_ENTRIES * sizeof(pgentry_t));
    memset(paddr_to_virt_kern(virt_to_paddr(l2_pt_entries)), 0,
           L2_PT_ENTRIES * sizeof(pgentry_t));
    memset(paddr_to_virt_kern(virt_to_paddr(l1_pt_entries)), 0,
           L1_PT_ENTRIES * sizeof(pgentry_t));
}

void __noreturn __text_init kernel_start(uint32_t multiboot_magic,
                                         multiboot_info_t *mbi) {
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

    init_acpi();

    init_traps(0);

    zap_boot_mappings();

    init_slab();

    init_apic(APIC_MODE_XAPIC);

    init_tasks();

    smp_init();

    /* Jump from .text.init section to .text */
    asm volatile("push %0; ret" ::"r"(&kernel_main));

    UNREACHABLE();
}
