#include <ktf.h>
#include <lib.h>
#include <console.h>
#include <setup.h>
#include <multiboot.h>

extern void _long_to_real(void);

void kernel_main(void) {
    printk("\nKTF - KVM Test Framework!\n\n");

    display_memory_map();
    display_multiboot_mmap();

    _long_to_real();
    printk("\n After long_to_real\n");

    dump_pagetables();

    test_main();

    while(1)
        halt();
}
