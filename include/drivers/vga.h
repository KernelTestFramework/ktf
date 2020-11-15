/*
 * Copyright © 2020 Amazon.com, Inc. or its affiliates.
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
#ifndef KTF_DRV_VGA_H
#define KTF_DRV_VGA_H

#include <ktf.h>

#define VGA_COLOR(bg, fg) (((bg) << 4) | (fg))

enum vga_color {
    VGA_BLACK = 0x0,
    VGA_BLUE = 0x1,
    VGA_GREEN = 0x2,
    VGA_CYAN = 0x3,
    VGA_RED = 0x4,
    VGA_MAGNETA = 0x5,
    VGA_BROWN = 0x6,
    VGA_LIGHT_GRAY = 0x7,
    VGA_GRAY = 0x8,
    VGA_LIGHT_BLUE = 0x9,
    VGA_LIGHT_GREEN = 0xa,
    VGA_LIGHT_CYAN = 0xb,
    VGA_LIGHT_RED = 0xc,
    VGA_LIGHT_MAGNETA = 0xd,
    VGA_YELLOW = 0xe,
    VGA_WHITE = 0xf,
};
typedef enum vga_color vga_color_t;

#define VGA_START_ADDR 0xB8000
#define VGA_END_ADDR   0xBFFFF
#define VGA_ROWS       25
#define VGA_COLS       80
#define VGA_SCREENS    10

extern void vga_scroll_up(void);
extern void vga_scroll_down(void);

extern void vga_write(const char *buf, size_t len, vga_color_t color);
#endif /* KTF_DRV_VGA_H */
