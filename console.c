/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <ktf.h>
#include <lib.h>
#include <setup.h>
#include <string.h>
#include <console.h>
#include <spinlock.h>

#include <drivers/vga.h>
#include <drivers/serial.h>

#include <smp/smp.h>

#define QEMU_CONSOLE   0x0e9
#define SERIAL_CONSOLE (com_ports[0])

#define VPRINTK_BUF_SIZE 1024

static console_callback_t console_callbacks[2];
static unsigned int num_console_callbacks;

static void vprintk(const char *fmt, va_list args) {
    static char buf[VPRINTK_BUF_SIZE];
    static spinlock_t lock = SPINLOCK_INIT;
    unsigned int i;
    int rc;

    spin_lock(&lock);

    rc = vsnprintf(buf, sizeof(buf), fmt, args);

    if ( rc > (int)sizeof(buf) )
        panic("vprintk() buffer overflow\n");

    for ( i = 0; i < num_console_callbacks; i++ )
        console_callbacks[i](buf, rc);

    spin_unlock(&lock);
}

void printk(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);
}

void putchar(int c) {
    putc(SERIAL_CONSOLE, c);
}

void serial_console_write(const char *buf, size_t len) {
    serial_write(SERIAL_CONSOLE, buf, len);
}

void qemu_console_write(const char *buf, size_t len) {
    puts(QEMU_CONSOLE, buf, len);
}

void vga_console_write(const char *buf, size_t len) {
    vga_write(buf, len, VGA_WHITE);
}

void register_console_callback(console_callback_t cb) {
    console_callbacks[num_console_callbacks++] = cb;
}

void __noreturn panic(const char *fmt, ...) {
    va_list args;

    printk("******************************\n");
    printk("CPU[%u] PANIC: ", smp_processor_id());

    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);

    printk("******************************\n");

    while(1)
        halt();
}
