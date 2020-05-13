#include <ktf.h>
#include <lib.h>
#include <console.h>
#include <setup.h>
#include <multiboot.h>

void kernel_main(void) {
    printk("\nKTF - KVM Test Framework!\n");

    display_multiboot_mmap();

    while(1)
        halt();
}
