#ifndef KTF_CONSOLE_H
#define KTF_CONSOLE_H

typedef void (*console_callback_t)(const char *buf, size_t len);

extern void printk(const char *fmt, ...);
extern void putchar(int c);

extern void serial_console_write(const char *buf, size_t len);
extern void qemu_console_write(const char *buf, size_t len);
extern void vga_console_write(const char *buf, size_t len);

extern void register_console_callback(console_callback_t func);

extern void panic(const char *fmt, ...);

#endif /* KTF_CONSOLE_H */
