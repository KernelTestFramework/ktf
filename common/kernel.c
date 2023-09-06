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
#include <acpi_ktf.h>
#include <console.h>
#include <drivers/keyboard.h>
#include <ktf.h>
#include <lib.h>
#include <multiboot2.h>
#include <percpu.h>
#include <sched.h>
#include <setup.h>
#ifdef KTF_PMU
#include <perfmon/pfmlib.h>
#endif

void reboot(void) {
    printk("Rebooting...\n");
    io_delay();

#ifdef KTF_ACPICA
    acpi_reboot();
#endif
    keyboard_reboot();
    hard_reboot();
}

static void __noreturn echo_loop(void) {
    while (1) {
        io_delay();
        keyboard_process_keys();
    }
}

void kernel_main(void) {
    printk("\nKTF - Kernel Test Framework!\n");

    zap_boot_mappings();
    if (opt_debug) {
        display_memory_map();
        display_multiboot_mmap();
    }

    execute_tasks();

    test_main(NULL);
    printk("All tasks done.\n");

    execute_tasks();

#ifdef KTF_PMU
    pfm_terminate();
#endif

#ifdef KTF_ACPICA
    if (opt_poweroff)
        acpi_power_off();
#endif
    echo_loop();
}
