#include <ktf.h>
#include <console.h>
#include <string.h>

void kernel_start(void) {
    register_console_callback(serial_console_write);

    printk("KTF - KVM Test Framework!\n");
}
