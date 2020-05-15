#ifndef KTF_DRV_VGA_H
#define KTF_DRV_VGA_H

#include <ktf.h>

#define VGA_COLOR(bg, fg)       (((bg) << 4) | (fg))

enum vga_color {
    VGA_BLACK          = 0x0,
    VGA_BLUE           = 0x1,
    VGA_GREEN          = 0x2,
    VGA_CYAN           = 0x3,
    VGA_RED            = 0x4,
    VGA_MAGNETA        = 0x5,
    VGA_BROWN          = 0x6,
    VGA_LIGHT_GRAY     = 0x7,
    VGA_GRAY           = 0x8,
    VGA_LIGHT_BLUE     = 0x9,
    VGA_LIGHT_GREEN    = 0xa,
    VGA_LIGHT_CYAN     = 0xb,
    VGA_LIGHT_RED      = 0xc,
    VGA_LIGHT_MAGNETA  = 0xd,
    VGA_YELLOW         = 0xe,
    VGA_WHITE          = 0xf,
};
typedef enum vga_color vga_color_t;

#define VGA_START_ADDR 0xB8000
#define VGA_END_ADDR   0xBFFFF
#define VGA_ROWS       25
#define VGA_COLS       80

extern void vga_write(const char *buf, size_t len, vga_color_t color);
#endif /* KTF_DRV_VGA_H */
