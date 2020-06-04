#include <ktf.h>
#include <console.h>

static void __user_text func(void *arg) {
}

void test_main(void) {
    printk("\nTest:\n");

    usermode_call(func, NULL);

    printk("Test done\n");
}
