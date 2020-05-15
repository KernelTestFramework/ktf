#include <ktf.h>
#include <page.h>
#include <string.h>
#include <drivers/vga.h>

#define MAX_ROWS VGA_ROWS
#define MAX_COLS (2 * VGA_COLS)

static uint8_t vga_buffer[MAX_ROWS][MAX_COLS];

static inline void write_vga_buffer(void) {
    void *vga_memory = paddr_to_virt_kern(VGA_START_ADDR);

    memcpy(vga_memory, vga_buffer, sizeof(vga_buffer));
}

void vga_write(const char *buf, size_t len, vga_color_t color) {
    static int row = 0, col = 0;

    for (int i = 0; i < len; i++) {
        char c = buf[i];

        /* Newline on LF or when columns limit is hit */
        if ((col > 0 && (col % (MAX_COLS - 2)) == 0) || c == '\n') {
            col = 0;
            row++;
        }

        /* Scroll up one row when hit end of VGA area */
        if (row == (MAX_ROWS - 1)) {
            memcpy(vga_buffer[0], vga_buffer[1], sizeof(vga_buffer) - MAX_COLS);
            memset(vga_buffer[MAX_ROWS - 1], 0x00, MAX_COLS);
            col = 0;
            row--;
        }

        if (c == '\n')
            continue;

        vga_buffer[row][col++] = buf[i];
        vga_buffer[row][col++] = color;
    }

    write_vga_buffer();
}

