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
#ifndef KTF_CONSOLE_H
#define KTF_CONSOLE_H

typedef void (*console_callback_t)(const char *buf, size_t len);

extern void printk(const char *fmt, ...);
#define dprintk(fmt, ...) do {                   \
    if (opt_debug) printk((fmt), ##__VA_ARGS__); \
} while(0)

extern void putchar(int c);

extern void serial_console_write(const char *buf, size_t len);
extern void qemu_console_write(const char *buf, size_t len);
extern void vga_console_write(const char *buf, size_t len);

extern void register_console_callback(console_callback_t func);

extern void panic(const char *fmt, ...);

#endif /* KTF_CONSOLE_H */
