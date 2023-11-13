/*
 * Copyright © 2020 Amazon.com, Inc. or its affiliates.
 * Copyright © 2014,2015 Citrix Systems Ltd.
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <console.h>
#include <ktf.h>
#include <lib.h>
#include <setup.h>
#include <spinlock.h>
#include <string.h>

#include <drivers/fb.h>
#include <drivers/serial.h>
#include <drivers/vga.h>

#include <smp/smp.h>

#define VPRINTK_BUF_SIZE 1024

static console_callback_entry_t console_callbacks[2];
static unsigned int num_console_callbacks;

void vprintk(const char *fmt, va_list args) {
    static char buf[VPRINTK_BUF_SIZE];
    static spinlock_t lock = SPINLOCK_INIT;
    unsigned int i;
    int rc;

    spin_lock(&lock);

    rc = vsnprintf(buf, sizeof(buf), fmt, args);

    if (rc > (int) sizeof(buf))
        panic("vprintk() buffer overflow");

    for (i = 0; i < num_console_callbacks; i++) {
        void *arg = console_callbacks[i].arg;

        console_callbacks[i].cb(arg, buf, rc);
    }

    spin_unlock(&lock);
}

void printk(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);
}

#ifdef KTF_ACPICA
void AcpiOsVprintf(const char *Format, va_list args) {
    vprintk(Format, args);
}

void AcpiOsPrintf(const char *Format, ...) {
    va_list args;

    va_start(args, Format);
    vprintk(Format, args);
    va_end(args);
}
#endif

void serial_console_write(void *arg, const char *buf, size_t len) {
    io_port_t port = (io_port_t) _ul(arg);

    serial_write(port, buf, len);
}

void qemu_console_write(void *arg, const char *buf, size_t len) {
    io_port_t port = (io_port_t) _ul(arg);

    puts(port, buf, len);
}

void vga_console_write(void *vga_memory, const char *buf, size_t len) {
    vga_write(vga_memory, buf, len, VGA_WHITE);
}

void fb_console_write(void *fb_memory, const char *buf, size_t len) {
    fb_write(fb_memory, buf, len, FB_WHITE);
}

static inline bool is_console_callback_registered(console_callback_t cb, void *arg) {
    for (unsigned int i = 0; i < num_console_callbacks; i++) {
        if (console_callbacks[i].cb == cb && console_callbacks[i].arg == arg)
            return true;
    }

    return false;
}

void register_console_callback(console_callback_t cb, void *arg) {
    if (is_console_callback_registered(cb, arg))
        return;

    console_callbacks[num_console_callbacks].cb = cb;
    console_callbacks[num_console_callbacks++].arg = arg;
}

void panic(const char *fmt, ...) {
    va_list args;

    printk("************** PANIC **************\n");
    printk("CPU[%u]: ", smp_processor_id());

    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);

    printk("\n***********************************\n");

    while (1)
        halt();
}

void warning(const char *fmt, ...) {
    va_list args;

    printk("************* WARNING *************\n");
    printk("CPU[%u]: ", smp_processor_id());

    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);

    printk("\n***********************************\n");
}
