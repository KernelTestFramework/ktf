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

#define KEYBOARD_PORT1_IRQ 0x01
#define KEYBOARD_PORT2_IRQ 0x04 /* Second port */
#define KEYBOARD_PORT1_IRQ0_OFFSET                                                       \
    (PIC_IRQ0_OFFSET + KEYBOARD_PORT1_IRQ) /* keyboard first channel irq offset */
#define KEYBOARD_PORT2_IRQ0_OFFSET (PIC_IRQ0_OFFSET + KEYBOARD_PORT2_IRQ)

#define KEYBOARD_STATUS_OUT_FULL 0x01 /* bit set when the keyboard buffer is full */

typedef enum {
    KEYBOARD_CMD_WRITE_CONFIGURATION = 0x60, /* Write configuration byte */
    KEYBOARD_CMD_READ_CONFIGURATION = 0x20,  /* Read configuration byte */
    KEYBOARD_CMD_SELF_TEST = 0xAA,
    KEYBOARD_CMD_TEST_PORT_1 = 0xAB,
    KEYBOARD_CMD_TEST_PORT_2 = 0xA9,
    KEYBOARD_CMD_DISABLE_PORT_1 = 0xAD,
    KEYBOARD_CMD_DISABLE_PORT_2 = 0xA7,
    KEYBOARD_CMD_ENABLE_PORT_1 = 0xAE,
    KEYBOARD_CMD_ENABLE_PORT_2 = 0xA8,
} keyboard_cmd_t;

#define KEYBOARD_RES_SELF_TEST 0x55

union keyboard_controller_config {
    struct {
        /* clang-format off */
        uint8_t port1_int : 1,
                port2_int : 1,
                sys_flag : 1,
                zero0 : 1,
                clock1 : 1,
                clock2 : 1,
                translation : 1,
                zero1 : 1;
        /* clang-format on */
    } __packed;
    uint8_t config;
};
typedef union keyboard_controller_config keyboard_controller_config_t;

#define KEY_BUF 80 /* size of internal input buffer */

#define SCAN_RELEASE_MASK 0x80 /* bit set on key release */

#define KEY_NULL 0x00 /* no key */

enum scan_code {
    SCAN_NULL = 0x00,
    SCAN_ESC = 0x01,
    SCAN_ENTER = 0x1c,
    SCAN_CTRL = 0x1d,
    SCAN_LSHIFT = 0x2a,
    SCAN_RSHIFT = 0x36,
    SCAN_ALT = 0x38,
    SCAN_CAPS = 0x3a,
    SCAN_F1 = 0x3b,
    SCAN_F2 = 0x3c,
    SCAN_F3 = 0x3d,
    SCAN_F4 = 0x3e,
    SCAN_F5 = 0x3f,
    SCAN_F6 = 0x40,
    SCAN_F7 = 0x41,
    SCAN_F8 = 0x42,
    SCAN_F9 = 0x43,
    SCAN_F10 = 0x44,
    SCAN_F11 = 0x55,
    SCAN_F12 = 0x58,
    SCAN_NUMLOCK = 0x45,
    SCAN_SCROLLLOCK = 0x46,
    SCAN_HOME = 0x47,
    SCAN_UP = 0x48,
    SCAN_PAGEUP = 0x49,
    SCAN_LEFT = 0x4b,
    SCAN_KEYPAD5 = 0x4c,
    SCAN_RIGHT = 0x4d,
    SCAN_END = 0x4f,
    SCAN_DOWN = 0x50,
    SCAN_PAGEDOWN = 0x51,
    SCAN_INS = 0x52,
    SCAN_DEL = 0x53
};
typedef enum scan_code scan_code_t;

/* External Declarations */

extern void init_keyboard(uint8_t dst_cpus);
extern void keyboard_interrupt_handler(void);
extern unsigned int keyboard_process_keys(void);

#endif
