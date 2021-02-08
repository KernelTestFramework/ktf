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
#include <apic.h>
#include <drivers/keyboard.h>
#include <drivers/pic.h>
#include <drivers/vga.h>
#include <ioapic.h>
#include <lib.h>
#include <string.h>

/* clang-format off */
static const unsigned char key_map[] = { /* map scan code to key */
    0xff,        SCAN_ESC,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
    '\b',        '\t',      'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
    SCAN_ENTER,  SCAN_CTRL, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    SCAN_LSHIFT, '\\',      'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    SCAN_RSHIFT, 0x37, SCAN_ALT, ' ', SCAN_CAPS,
    SCAN_F1, SCAN_F2, SCAN_F3, SCAN_F4, SCAN_F5, SCAN_F6, SCAN_F7, SCAN_F8, SCAN_F9, SCAN_F10,
    SCAN_NUMLOCK, SCAN_SCROLLLOCK, SCAN_HOME, SCAN_UP, SCAN_PAGEUP, 0x4a, SCAN_LEFT,
    SCAN_KEYPAD5, SCAN_RIGHT, 0x4e, SCAN_END, SCAN_DOWN, SCAN_PAGEDOWN, SCAN_INS, SCAN_DEL,
    0x54, SCAN_F11, 0x56, 0x57, SCAN_F12
};

static const unsigned char key_map_up[] = { /* map scan code to upper key */
    0xff,        SCAN_ESC,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
    '\b',        '\t',      'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
    SCAN_ENTER,  SCAN_CTRL, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    SCAN_LSHIFT, '|',       'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?'};
/* clang-format on */

struct keyboard_state {
    bool shift, caps, ctrl, alt;

    char buf[KEY_BUF];
    unsigned curr;
    unsigned init;
};
typedef struct keyboard_state keyboard_state_t;

static keyboard_state_t keyboard_state;

void init_keyboard(uint8_t dst_cpus) {
    printk("Initializing keyboard driver\n");

    /* Disable devices */
    outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_DISABLE_PORT_1);
    outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_DISABLE_PORT_2);

    /* Flush output buffer */
    while (inb(KEYBOARD_PORT_DATA) & KEYBOARD_STATUS_OUT_FULL)
        ; /* discard leftover bytes */

    /* Controller configuration */
    keyboard_controller_config_t current_status;
    bool dual_channel;

    /* Read controller config */
    outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_READ_CONFIGURATION);
    current_status.config = inb(KEYBOARD_PORT_DATA);

    dual_channel = current_status.clock2 == 0 ? 1 : 0; /* second channel enabled if 0 */

    dprintk("Current PS/2 status before: %x\n", current_status.config);

    /* Disable IRQs and translation */
    current_status.port1_int = 0;
    current_status.port2_int = 0;
    current_status.translation = 0;

    /* Write controller config */
    outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_WRITE_CONFIGURATION);
    outb(KEYBOARD_PORT_DATA, current_status.config);

    dprintk("Current PS/2 status after mods: %x\n", current_status.config);
    dprintk("PS/2 dual channel? %d\n", dual_channel);

    /* Controller self test */
    outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_SELF_TEST);
    if (inb(KEYBOARD_PORT_DATA) != KEYBOARD_RES_SELF_TEST) {
        printk("Self test did not succeed\n");
        return;
    }

    /* Determine whether second channel exists */
    if (dual_channel) {
        outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_ENABLE_PORT_2);
        outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_READ_CONFIGURATION);
        current_status.config = inb(KEYBOARD_PORT_DATA);
        dual_channel = current_status.clock2 == 0 ? 1 : 0;
    }
    if (dual_channel)
        outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_DISABLE_PORT_2);

    /* Interface tests */
    int port1, port2 = 0;
    outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_TEST_PORT_1);
    port1 = inb(KEYBOARD_PORT_DATA) == 0 ? 1 : 0;
    if (dual_channel) {
        outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_TEST_PORT_2);
        port2 = inb(KEYBOARD_PORT_DATA) == 0 ? 1 : 0;
    }

    dprintk("Port1 available? %d - port2 available? %d\n", port1, port2);
    if (!port1 && !port2) {
        printk("No available PS/2 working ports\n");
        return;
    }

    /* Enable devices */
    if (port1) {
        dprintk("Keyboard: enabling first channel\n");
        current_status.port1_int = 1;
        current_status.clock2 = 1; /* disable second port clock */
        current_status.translation = 1;
        outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_WRITE_CONFIGURATION);
        outb(KEYBOARD_PORT_DATA, current_status.config);

        configure_isa_irq(KEYBOARD_PORT1_IRQ, KEYBOARD_PORT1_IRQ0_OFFSET,
                          IOAPIC_DEST_MODE_PHYSICAL, dst_cpus);
        outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_ENABLE_PORT_1);
    }
    else {
        dprintk("Keyboard: enabling second channel\n");
        current_status.port2_int = 1;
        outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_WRITE_CONFIGURATION);
        outb(KEYBOARD_PORT_DATA, current_status.config);

        configure_isa_irq(KEYBOARD_PORT2_IRQ, KEYBOARD_PORT2_IRQ0_OFFSET,
                          IOAPIC_DEST_MODE_PHYSICAL, dst_cpus);
        outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_ENABLE_PORT_2);
    }

    memset(&keyboard_state, 0, sizeof(keyboard_state));
}

static char keyboard_scan_to_key(char scan) {
    if ((unsigned long) scan < sizeof(key_map)) {
        unsigned char key = key_map[(unsigned) scan];

        if (key >= 'a' && key <= 'z')
            return key + ('A' - 'a') * (keyboard_state.caps ^ keyboard_state.shift);
        else if (keyboard_state.shift && (unsigned long) scan < sizeof(key_map_up))
            return key_map_up[(unsigned) scan];

        return key;
    }

    return scan;
}

unsigned int keyboard_process_keys(void) {
    unsigned n = 0;
    unsigned char key, scan;

    while (keyboard_state.curr != keyboard_state.init) {
        scan = keyboard_state.buf[keyboard_state.init];

        key = keyboard_scan_to_key(scan);

        switch (key) {
        case SCAN_PAGEUP:
            if (keyboard_state.shift)
                vga_scroll_up();
            break;
        case SCAN_PAGEDOWN:
            if (keyboard_state.shift)
                vga_scroll_down();
            break;
        default:
            if (isprint(key))
                printk("%c", key);
            break;
        }

        keyboard_state.init = (keyboard_state.init + 1) % KEY_BUF;
        ++n;
    }
    return n;
}

void keyboard_interrupt_handler(void) {
    unsigned char status;

    status = inb(KEYBOARD_PORT_CMD);
    if (status & KEYBOARD_STATUS_OUT_FULL) {
        char scan = inb(KEYBOARD_PORT_DATA);
        int released = !!(scan & SCAN_RELEASE_MASK);

        scan = scan & (SCAN_RELEASE_MASK - 1);

        switch (scan) {
        case SCAN_LSHIFT:
        case SCAN_RSHIFT:
            keyboard_state.shift = !released;
            break;
        case SCAN_CAPS:
            if (!released)
                keyboard_state.caps = !keyboard_state.caps;
            break;
        case SCAN_CTRL:
            keyboard_state.ctrl = !released;
            break;
        case SCAN_ALT:
            keyboard_state.alt = !released;
            break;
        default:
            if (!released) {
                if ((long unsigned) scan < sizeof(key_map)) {
                    keyboard_state.buf[keyboard_state.curr] = scan;
                    keyboard_state.curr = (keyboard_state.curr + 1) % KEY_BUF;
                }
                break;
            }
        }
    }

    apic_EOI();
}
