/*
 * Copyright © 2022 Open Source Security, Inc.
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
#include <drivers/fb.h>
#include <drivers/logo.h>
#include <ktf.h>
#include <multiboot2.h>
#include <pagetable.h>
#include <string.h>

extern uint64_t fonts[];

#define FONT_SIZE           sizeof(fonts[0])
#define TEXT_AREA_FIRST_ROW (((LOGO_HEIGHT + FONT_SIZE) / FONT_SIZE) * FONT_SIZE)

static bool has_fb = false;
static bool scroll = false;

static uint32_t width;
static uint32_t height;
static uint32_t pitch;
static uint8_t bpp;

static size_t buffer_size;
static size_t banner_size;
static void *video_memory;

static void *first_line_addr;
static void *last_line_addr;
static uint64_t line_width;

static void (*put_pixel)(uint32_t x, uint32_t y, uint32_t color);

static void map_fb_area(paddr_t start, size_t size) {
    vmap_range(start, size, L1_PROT_NOCACHE, VMAP_KERNEL | VMAP_IDENT);
}

static void put_pixel8(uint32_t x, uint32_t y, uint32_t color) {
    *(uint8_t *) (video_memory + (y * pitch) + x) = color;
}

static void put_pixel16(uint32_t x, uint32_t y, uint32_t color) {
    *(uint16_t *) (video_memory + (y * pitch) + (x * 2)) = color;
}

static void put_pixel24(uint32_t x, uint32_t y, uint32_t color) {
    uint32_t *px = video_memory + (y * pitch) + (x * 3);

    *px = (color & 0x00ffffff) | (*px & 0xff000000);
}

static void put_pixel32(uint32_t x, uint32_t y, uint32_t color) {
    *(uint32_t *) (video_memory + (y * pitch) + (x * 4)) = color;
}

void init_framebuffer(const struct multiboot2_tag_framebuffer *fb) {
    video_memory = (void *) fb->common.framebuffer_addr;
    if (!video_memory)
        return;

    width = fb->common.framebuffer_width;
    height = fb->common.framebuffer_height;
    pitch = fb->common.framebuffer_pitch;
    bpp = fb->common.framebuffer_bpp;

    buffer_size = (size_t) width * height;
    banner_size = (size_t) width * (LOGO_HEIGHT + FONT_SIZE);

    switch (bpp) {
    case 8:
        put_pixel = put_pixel8;
        break;
    case 15:
    case 16:
        put_pixel = put_pixel16;
        buffer_size *= 2;
        banner_size *= 2;
        break;
    case 24:
        put_pixel = put_pixel24;
        buffer_size *= 3;
        banner_size *= 3;
        break;
    case 32:
        put_pixel = put_pixel32;
        buffer_size *= 4;
        banner_size *= 4;
        break;
    default:
        warning("FB: Unsupported framebuffer BPP: %u", bpp);
        has_fb = false;
        return;
    }

    line_width = pitch * FONT_SIZE;
    first_line_addr = (uint8_t *) video_memory + TEXT_AREA_FIRST_ROW * pitch;
    last_line_addr = (uint8_t *) video_memory + buffer_size - line_width;

    has_fb = true;
}

bool setup_framebuffer(void) {
    if (!has_fb)
        return false;

    map_fb_area(_paddr(video_memory), buffer_size);
    memset(video_memory, 0, buffer_size);

    register_console_callback(fb_console_write, video_memory);
    scroll = opt_fb_scroll;
    return true;
}

void put_char(char c, uint32_t x, uint32_t y, uint32_t color) {
    uint64_t font = fonts[(uint8_t) c];

    for (unsigned int yy = 0; yy < FONT_SIZE; yy++) {
        for (unsigned int xx = 0; xx < FONT_SIZE; xx++, font >>= 1) {
            if (font & 1)
                put_pixel(x + xx, y + yy, color);
        }
    }
}

void draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color) {
    while (x1 <= x2 || y1 <= y2) {
        put_pixel(x1, y1, color);
        if (x1 <= x2)
            x1++;
        if (y1 <= y2)
            y1++;
    }
}

void draw_logo(void) {
    uint32_t *px = (uint32_t *) logo;

    for (int y = LOGO_HEIGHT; y > 0; y--) {
        for (int x = 0; x < LOGO_WIDTH; x++)
            put_pixel(x, y, *px++);
    }

    draw_line(0, LOGO_HEIGHT + 2, width, LOGO_HEIGHT + 2, FB_WHITE);
}

void set_fb_scroll(bool state) {
    scroll = state;
}

static inline void scroll_up_line(void) {
    memmove(first_line_addr, first_line_addr + line_width,
            last_line_addr - first_line_addr);
}

static inline void clear_screen(void) {
    memset(first_line_addr, 0, last_line_addr - first_line_addr);
}

void fb_write(void *fb_addr, const char *buf, size_t len, uint32_t color) {
    static uint32_t row = TEXT_AREA_FIRST_ROW, col = 0;

    for (unsigned int i = 0; i < len; i++) {
        char c = buf[i];

        if ((col + FONT_SIZE) >= width || c == '\n') {
            row += FONT_SIZE;
            col = 0;
        }

        if ((row + FONT_SIZE) >= height) {
            if (scroll) {
                scroll_up_line();
                row -= FONT_SIZE;
            }
            else {
                clear_screen();
                row = TEXT_AREA_FIRST_ROW;
            }
            col = 0;
        }

        if (c == '\r') {
            col = 0;
            continue;
        }

        if (c == '\n')
            continue;

        put_char(c, col, row, color);
        col += FONT_SIZE;
    }
}
