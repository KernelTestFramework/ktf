#include <ktf.h>
#include <console.h>
#include <string.h>
#include <multiboot.h>

void kernel_start(multiboot_info_t *mbi, const char *cmdline) {
    register_console_callback(serial_console_write);

    printk("KTF - KVM Test Framework!\n");
    printk("Multiboot cmdline: %s\n", cmdline);
}
