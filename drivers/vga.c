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
#include <drivers/vga.h>
#include <ktf.h>
#include <page.h>
#include <string.h>

#define MAX_ROWS VGA_ROWS
#define MAX_COLS (2 * VGA_COLS)

static unsigned scroll_screen = 0;

static uint8_t vga_buffer[VGA_SCREENS][MAX_ROWS][MAX_COLS];

static inline void write_vga_buffer(int cur_screen) {
    void *vga_memory = paddr_to_virt_kern(VGA_START_ADDR);

    memcpy(vga_memory, vga_buffer[cur_screen], sizeof(vga_buffer[cur_screen]));
}

void vga_scroll_down(void) {
    if (scroll_screen < (VGA_SCREENS - 1))
        write_vga_buffer(++scroll_screen);
}

void vga_scroll_up(void) {
    if (scroll_screen > 0)
        write_vga_buffer(--scroll_screen);
}

void vga_write(const char *buf, size_t len, vga_color_t color) {
    static int screen = 0, row = 0, col = 0;

    for (unsigned int i = 0; i < len; i++) {
        char c = buf[i];

        /* Newline on LF or when columns limit is hit */
        if ((col > 0 && (col % (MAX_COLS - 2)) == 0) || c == '\n') {
            col = 0;
            row++;
        }

        /* Go to the next screen when hit end of VGA area */
        if (row == (MAX_ROWS - 1)) {
            screen = (screen + 1) % VGA_SCREENS;
            memset(vga_buffer[screen], 0x00, sizeof(vga_buffer[screen]));
            row = col = 0;
        }

        if (c == '\n')
            continue;

        vga_buffer[screen][row][col++] = buf[i];
        vga_buffer[screen][row][col++] = color;
    }

    scroll_screen = screen;
    write_vga_buffer(screen);
}
