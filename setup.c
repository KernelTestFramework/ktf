#include <ktf.h>
#include <page.h>
#include <traps.h>
#include <console.h>
#include <multiboot.h>

/*
 * KTF Stack layout:
 *
 * kernel_stack[page 5] Emergency stack
 * kernel_stack[page 4] Exception stack
 * kernel_stack[page 1-3] Regular stack
 */
uint8_t kernel_stack[5 * PAGE_SIZE] __aligned(PAGE_SIZE) __data;

char kernel_cmdline[PAGE_SIZE];

static void init_console(void) {
    register_console_callback(serial_console_write);
}

void __noreturn __text_init kernel_start(multiboot_info_t *mbi) {

    /* Indentity mapping is still on, so fill in multiboot structures */
    init_multiboot(mbi);

    /* Initialize console early */
    init_console();

    /* TODO: Exception tables */

    init_traps();

    /* TODO PerCPU support */

    /* TODO: SMP support */

    /* Jump from .text.init section to .text */
    asm volatile("push %0; ret" :: "r" (&kernel_main));

    UNREACHABLE();
}
