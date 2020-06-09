#include <ktf.h>
#include <console.h>

static int __user_text func(void *arg) {
    return 0;
}

void test_main(void) {
    printk("\nTest:\n");

    usermode_call(func, NULL);

    printk("Test done\n");
}
