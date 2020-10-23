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
#include <drivers/keyboard.h>
#include <drivers/pic.h>
#include <lib.h>
#include <string.h>

static struct {
    int shift, caps, ctrl, alt;

    char buf[KEY_BUF];
    unsigned curr;
    unsigned init;
} keyboard_state;

void init_keyboard() {
    printk("Initializing keyboard driver\n");

    /* Disable devices */
    outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_DISABLE_PORT_0);
    outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_DISABLE_PORT_1);

    /* Flush output buffer */
    while (inb(KEYBOARD_PORT_DATA) & KEYBOARD_STATUS_OUT_FULL)
        ; /* discard leftover bytes */

    /* Controller configuration */
    char current_status;
    int dual_channel;

    /* Read controller config */
    outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_READ_CONFIGURATION);
    current_status = inb(KEYBOARD_PORT_DATA);

    dual_channel = !!(current_status & (1 << KEYBOARD_CONTROLLER_CONFIG_BIT_CLOCK_1));
    printk("Current PS/2 status before: %x\n", current_status);
    /* Disable IRQs and translation */
    current_status = current_status &
                     ~(1 << KEYBOARD_CONTROLLER_CONFIG_BIT_PORT_0_INTERRUPT) &
                     ~(1 << KEYBOARD_CONTROLLER_CONFIG_BIT_PORT_1_INTERRUPT) &
                     ~(1 << KEYBOARD_CONTROLLER_CONFIG_BIT_TRANSLATION);
    /* Write controller config */
    outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_WRITE_CONFIGURATION);
    outb(KEYBOARD_PORT_DATA, current_status);

    printk("Current PS/2 status after mods: %x\n", current_status);
    printk("PS/2 dual channel? %d\n", dual_channel);

    /* Controller self test */
    outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_SELF_TEST);
    if (inb(KEYBOARD_PORT_DATA) != KEYBOARD_RES_SELF_TEST) {
        printk("Self test did not succed\n");
        return;
    }

    /* Determine whether second channel exists */
    if (dual_channel) {
        printk("Enabling second PS/2 port\n");
        outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_ENABLE_PORT_1);
        outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_READ_CONFIGURATION);
        current_status = inb(KEYBOARD_PORT_DATA);
        dual_channel =
            (current_status & (1 << KEYBOARD_CONTROLLER_CONFIG_BIT_CLOCK_1)) == 0;
    }
    if (dual_channel)
        outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_DISABLE_PORT_1);

    /* Interface tests */
    int port1, port2 = 0;
    outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_TEST_PORT_0);
    port1 = inb(KEYBOARD_PORT_DATA) == 0;
    if (dual_channel) {
        outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_TEST_PORT_1);
        port2 = inb(KEYBOARD_PORT_DATA) == 0;
    }

    printk("Port1 available? %d - port2 available? %d\n", port1, port2);
    if (!port1 && !port2) {
        printk("No available PS/2 working ports\n");
        return;
    }

    /* Enable devices */
    if (port1) {
        printk("Keyboard: enabling first channel\n");
        current_status = current_status |
                         (1 << KEYBOARD_CONTROLLER_CONFIG_BIT_PORT_0_INTERRUPT) |
                         (1 << KEYBOARD_CONTROLLER_CONFIG_BIT_CLOCK_1) |
                         (1 << KEYBOARD_CONTROLLER_CONFIG_BIT_TRANSLATION);
        outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_WRITE_CONFIGURATION);
        outb(KEYBOARD_PORT_DATA, current_status);

        pic_enable_irq(PIC1_DEVICE_SEL, KEYBOARD_IRQ);
        outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_ENABLE_PORT_0);
    }
    else {
        printk("Keyboard: enabling second channel\n");
        current_status =
            current_status | (1 << KEYBOARD_CONTROLLER_CONFIG_BIT_PORT_1_INTERRUPT);
        outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_WRITE_CONFIGURATION);
        outb(KEYBOARD_PORT_DATA, current_status);

        pic_enable_irq(PIC1_DEVICE_SEL, KEYBOARD_IRQ_2);
        outb(KEYBOARD_PORT_CMD, KEYBOARD_CMD_ENABLE_PORT_1);
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

        if (isprint(key))
            printk("%c", key);

        keyboard_state.init = (keyboard_state.init + 1) % KEY_BUF;
        ++n;
    }
    return n;
}

void keyboard_interrupt_handler(void) {
    unsigned char status;
    int released;

    status = inb(KEYBOARD_PORT_CMD);
    if (status & KEYBOARD_STATUS_OUT_FULL) {
        char scan = inb(KEYBOARD_PORT_DATA);

        released = !!(scan & SCAN_RELEASE_MASK);
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

    outb(PIC1_PORT_CMD, PIC_EOI);
}
