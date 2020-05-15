#include <ktf.h>
#include <lib.h>
#include <setup.h>
#include <string.h>
#include <console.h>

#define QEMU_CONSOLE   0x0e9
#define SERIAL_CONSOLE (com_ports[0])

#define VPRINTK_BUF_SIZE 1024

static console_callback_t console_callbacks[2];
static unsigned int num_console_callbacks;

static inline void putc(int c, int port) {
    asm volatile("outb %%al, %%dx"
                 : "+a" (c)
                 : "d" (port));
}

static inline void puts(const char *buf, size_t len, int port) {
    asm volatile("rep; outsb"
                 : "+S" (buf), "+c" (len)
                 : "d" (port));
}

static void vprintk(const char *fmt, va_list args) {
    static char buf[VPRINTK_BUF_SIZE];
    unsigned int i;
    int rc;

    rc = vsnprintf(buf, sizeof(buf), fmt, args);

    if ( rc > (int)sizeof(buf) )
        panic("vprintk() buffer overflow\n");

    for ( i = 0; i < num_console_callbacks; i++ )
        console_callbacks[i](buf, rc);
}

void printk(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);
}

void putchar(int c) {
    putc(c, SERIAL_CONSOLE);
}

void serial_console_write(const char *buf, size_t len) {
    puts(buf, len, SERIAL_CONSOLE);
}

void qemu_console_write(const char *buf, size_t len) {
    puts(buf, len, QEMU_CONSOLE);
}

void register_console_callback(console_callback_t cb) {
    console_callbacks[num_console_callbacks++] = cb;
}

void __noreturn panic(const char *fmt, ...) {
    va_list args;

    printk("******************************\n");
    printk("PANIC: ");

    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);

    printk("******************************\n");

    while(1)
        halt();
}
