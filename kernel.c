#include <ktf.h>
#include <lib.h>
#include <console.h>
#include <setup.h>
#include <multiboot.h>

extern void _long_to_real(void);

void kernel_main(void) {
    printk("\nKTF - Kernel Test Framework!\n\n");

    if (kernel_cmdline)
        printk("Command line: %s\n", kernel_cmdline);

    display_memory_map();
    display_multiboot_mmap();

    _long_to_real();
    dprintk("\n After long_to_real\n");

    test_main();

    while(1)
        halt();
}
