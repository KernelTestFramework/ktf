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
#include <cmdline.h>
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

bool opt_debug = false;
bool_cmd("debug", opt_debug);

io_port_t com_ports[2];

const char *kernel_cmdline;

static __text_init int parse_bool(const char *s) {
    if (!strcmp("no", s) || !strcmp("off", s) || !strcmp("false", s) ||
        !strcmp("disable", s) || !strcmp("0", s))
        return 0;

    if (!strcmp("yes", s) || !strcmp("on", s) || !strcmp("true", s) ||
        !strcmp("enable", s) || !strcmp("1", s))
        return 1;

    return -1;
}

void __text_init cmdline_parse(const char *cmdline) {
    static __bss_init char opt[PAGE_SIZE];
    char *optval, *optkey, *q;
    const char *p = cmdline;
    struct ktf_param *param;

    if (cmdline == NULL)
        return;

    for (;;) {
        p = string_trim_whitspace(p);

        if (iseostr(*p))
            break;

        q = optkey = opt;
        while ((!isspace(*p)) && (!iseostr(*p))) {
            ASSERT(_ul(q - opt) < sizeof(opt) - 1);
            *q++ = *p++;
        }
        *q = '\0';

        /* split on '=' */
        optval = strchr(opt, '=');
        if (optval != NULL)
            *optval++ = '\0';
        else
            /* assume a bool later */
            optval = opt;

        for (param = __start_cmdline; param < __end_cmdline; param++) {
            if (strcmp(param->name, optkey))
                continue;

            switch (param->type) {
            case STRING:
                strcpy(param->var, optval);
                break;
            case ULONG:
                *(unsigned long *) param->var = strtoul(optval, NULL, 0);
                break;
            case BOOL:
                *(bool *) param->var =
                    !!parse_bool(!strcmp(optval, optkey) ? "1" : optval);
                break;
            default:
                panic("Unkown cmdline type detected...");
                break;
            }
        }
    }
}

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

void zap_boot_mappings(void) {
    for_each_memory_range (r) {
        if (r->base == VIRT_IDENT_BASE && IS_INIT_SECTION(r->name)) {
            if (strcmp(r->name, ".text.init"))
                memset(r->start, 0, r->end - r->start);

            for (mfn_t mfn = virt_to_mfn(r->start); mfn < virt_to_mfn(r->end); mfn++) {
                vunmap(mfn_to_virt(mfn), PAGE_ORDER_4K);
                reclaim_frame(mfn, PAGE_ORDER_4K);
            }
        }
    }
}

void __noreturn __text_init kernel_start(uint32_t multiboot_magic,
                                         multiboot_info_t *mbi) {
    /* Zero-out BSS sections */
    zero_bss();

    /* Initialize console early */
    init_console();

    init_boot_traps();

    if (multiboot_magic == MULTIBOOT_BOOTLOADER_MAGIC) {
        /* Indentity mapping is still on, so fill in multiboot structures */
        init_multiboot(mbi, &kernel_cmdline);
    }

    /* Parse commandline parameters */
    cmdline_parse(kernel_cmdline);

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

    init_slab();

    init_apic(APIC_MODE_XAPIC);

    init_tasks();

    init_smp();

    /* Jump from .text.init section to .text */
    asm volatile("push %0; ret" ::"r"(&kernel_main));

    UNREACHABLE();
}
