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
#include <multiboot.h>
#include <page.h>
#include <string.h>

extern uint64_t fonts[];

static uint32_t width;
static uint32_t height;
static uint32_t pitch;
static uint8_t bpp;

static size_t buffer_size;
static size_t banner_size;
static void *video_memory;

static void (*put_pixel)(uint32_t x, uint32_t y, uint32_t color);

static void map_fb_area(paddr_t start, size_t size) {
    for (mfn_t video_mfn = paddr_to_mfn(start); video_mfn < paddr_to_mfn(start + size);
         video_mfn++) {
        vmap_4k(mfn_to_virt(video_mfn), video_mfn, L1_PROT_NOCACHE);
        kmap_4k(video_mfn, L1_PROT_NOCACHE);
    }
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

bool init_framebuffer(const multiboot_info_t *mbi) {
    if (!mbi_has_framebuffer())
        return false;

    video_memory = (void *) mbi->framebuffer_addr;
    if (!video_memory)
        return false;

    width = mbi->framebuffer_width;
    height = mbi->framebuffer_height;
    pitch = mbi->framebuffer_pitch;
    bpp = mbi->framebuffer_bpp;

    buffer_size = (size_t) width * height;
    banner_size = (size_t) width * LOGO_HEIGHT;

    switch (bpp) {
    case 0 ... 7:
        printk("FB: Unsupported framebuffer BPP: %u\n", bpp);
        return false;
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
        BUG();
    }

    map_fb_area(mbi->framebuffer_addr, buffer_size);
    memset(video_memory, 0, buffer_size);

    register_console_callback(fb_console_write, video_memory);
    return true;
}

void put_char(char c, uint32_t x, uint32_t y, uint32_t color) {
    uint64_t font = fonts[(uint8_t) c];

    for (int yy = 0; yy < 8; yy++) {
        for (int xx = 0; xx < 8; xx++, font >>= 1) {
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

    draw_line(0, LOGO_HEIGHT + 4, width, LOGO_HEIGHT + 4, FB_WHITE);
}

static void clear_screen(void *fb_addr) {
    memset((uint8_t *) video_memory + banner_size, 0, buffer_size - banner_size);
}

void fb_write(void *fb_addr, const char *buf, size_t len, uint32_t color) {
    static uint32_t row = LOGO_HEIGHT + 8, col = 0;

    for (unsigned int i = 0; i < len; i++) {
        char c = buf[i];

        if ((col + 8) > width || c == '\n') {
            row += sizeof(fonts[0]);
            col = 0;
        }

        if ((row + 8) > height) {
            clear_screen(fb_addr);
            row = LOGO_HEIGHT + 8;
            col = 0;
        }

        if (c == '\n')
            continue;

        put_char(c, col, row, color);
        col += sizeof(fonts[0]);
    }
}
