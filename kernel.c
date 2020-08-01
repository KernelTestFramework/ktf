#include <ktf.h>
#include <lib.h>
#include <console.h>
#include <setup.h>
#include <multiboot.h>
#include <percpu.h>

extern void _long_to_real(void);

extern int usermode_call_asm(user_func_t fn, void *fn_arg, unsigned long ret2kern_sp, unsigned long user_stack);

void ret2kern_handler(void) {
    asm volatile("mov %%gs:(%0), %%" STR(_ASM_SP) :: "r" (offsetof(percpu_t, ret2kern_sp)));
}

int usermode_call(user_func_t fn, void *fn_arg) {
    return usermode_call_asm(fn, fn_arg, offsetof(percpu_t, ret2kern_sp), offsetof(percpu_t, user_stack));
}

void kernel_main(void) {
    printk("\nKTF - Kernel Test Framework!\n\n");

    if (kernel_cmdline)
        printk("Command line: %s\n", kernel_cmdline);

    display_memory_map();
    display_multiboot_mmap();

    if (opt_debug) {
        _long_to_real();
        printk("\n After long_to_real\n");
    }

    test_main();

    while(1)
        halt();
}
