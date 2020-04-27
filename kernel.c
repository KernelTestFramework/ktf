#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static void putc(int c) {
    asm volatile("outb %%al, %%dx"
                 : "+a" (c)
                 :"d" (0xe9));
}

static void com1_write(const char *buf, size_t len) {
    asm volatile("rep; outsb"
                 : "+S" (buf), "+c" (len)
                 : "d" (0x3f8));
}


static size_t strlen(const char *str) {
    size_t len = 0;

    while (str[len])
        len++;

    return len;
}

void kernel_start(void) {
    const char *str = "KTF - KVM Test Framework!\n";

    for (int i = 0; i < 80; i++)
        putc('-');
    putc('\n');

    com1_write(str, strlen(str));
}
