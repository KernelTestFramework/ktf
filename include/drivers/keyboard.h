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
#ifndef KTF_KEYBOARD_H
#define KTF_KEYBOARD_H

#include <drivers/pic.h>

#define KEYBOARD_PORT_CMD  0x64 /* keyboard command port */
#define KEYBOARD_PORT_DATA 0x60 /* keyboard data port */

#define KEYBOARD_IRQ0_OFFSET (PIC_IRQ0_OFFSET + 1) /* keyboard irq offset */
#define KEYBOARD_IRQ_UNMASK  (~2) /* mask with keyboard irq enabled, others disabled */

#define KEYBOARD_STATUS_OUT_FULL 1 /* bit set when the keyboard buffer is full */

#define KEY_BUF 80 /* size of internal input buffer */

#define SCAN_RELEASE_MASK 0x80 /* bit set on key release */

#define KEY_NULL 0x0 /* no key */

/* scan codes */
#define SCAN_NULL       0x0
#define SCAN_ESC        0x1
#define SCAN_ENTER      0x1c
#define SCAN_CTRL       0x1d
#define SCAN_LSHIFT     0x2a
#define SCAN_RSHIFT     0x36
#define SCAN_ALT        0x38
#define SCAN_CAPS       0x3a
#define SCAN_F1         0x3b
#define SCAN_F2         0x3c
#define SCAN_F3         0x3d
#define SCAN_F4         0x3e
#define SCAN_F5         0x3f
#define SCAN_F6         0x40
#define SCAN_F7         0x41
#define SCAN_F8         0x42
#define SCAN_F9         0x43
#define SCAN_F10        0x44
#define SCAN_F11        0x55
#define SCAN_F12        0x58
#define SCAN_NUMLOCK    0x45
#define SCAN_SCROLLLOCK 0x46
#define SCAN_HOME       0x47
#define SCAN_UP         0x48
#define SCAN_PAGEUP     0x49
#define SCAN_LEFT       0x4b
#define SCAN_KEYPAD5    0x4c
#define SCAN_RIGHT      0x4d
#define SCAN_END        0x4f
#define SCAN_DOWN       0x50
#define SCAN_PAGEDOWN   0x51
#define SCAN_INS        0x52
#define SCAN_DEL        0x53

void init_keyboard(void);
void keyboard_interrupt_handler(void);
unsigned int keyboard_process_keys(void);

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

#endif
