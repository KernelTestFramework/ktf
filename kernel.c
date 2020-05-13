#include <ktf.h>
#include <lib.h>
#include <console.h>
#include <string.h>
#include <setup.h>
#include <multiboot.h>

void kernel_start(multiboot_info_t *mbi) {

    arch_setup();

    printk("KTF - KVM Test Framework!\n");
    printk("Multiboot cmdline: %s\n", mbi->cmdline);

    while(1)
        halt();
}
