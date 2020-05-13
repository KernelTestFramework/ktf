#include <ktf.h>
#include <lib.h>
#include <console.h>
#include <setup.h>
#include <multiboot.h>

void kernel_main(void) {
    printk("\nKTF - KVM Test Framework!\n\n");

    display_memory_map();
    display_multiboot_mmap();

    dump_pagetables();

    test_main();

    while(1)
        halt();
}
