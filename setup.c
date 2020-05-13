#include <ktf.h>
#include <page.h>
#include <traps.h>
#include <console.h>

/*
 * KTF Stack layout:
 *
 * kernel_stack[page 5] Emergency stack
 * kernel_stack[page 4] Exception stack
 * kernel_stack[page 1-3] Regular stack
 */
uint8_t kernel_stack[5 * PAGE_SIZE] __aligned(PAGE_SIZE) __data;

void arch_setup(void) {
    register_console_callback(serial_console_write);

    /* TODO: Exception tables */

    init_traps();

    /* TODO PerCPU support */

    /* TODO: SMP support */
}
