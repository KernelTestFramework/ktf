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
#ifndef KTF_PIT_H
#define KTF_PIT_H

#include <drivers/pic.h>
#include <ktf.h>

#define PIT_IRQ         0x00                        /* IRQ line */
#define PIT_IRQ0_OFFSET (PIC_IRQ0_OFFSET + PIT_IRQ) /* IRQ relative to PIC master */

#define PIT_OUT_FREQUENCY 1193182 /* oscillator output frequency */
#define PIT_RELOAD        1000    /* 1ms */
#define PIT_FREQUENCY     (PIT_OUT_FREQUENCY / PIT_RELOAD)
#define PIT_DATA_PORT_CH0 0x40
#define PIT_COMMAND_PORT  0x43

#define PIT_CHANNEL_0        0
#define PIT_ACCESS_MODE_LOW  (1 << 4)
#define PIT_ACCESS_MODE_HIGH (1 << 5)
#define PIT_ACCESS_MODE_LH   (PIT_ACCESS_MODE_LOW | PIT_ACCESS_MODE_HIGH)

enum pit_operational_mode {
    PIT_OP_MODE_COUNT = 0x00 << 1,     /* interrupt on terminal count */
    PIT_OP_MODE_ONE_SHOT = 0x01 << 1,  /* hardware re-triggerable one-shot */
    PIT_OP_MODE_RATE = 0x02 << 1,      /* rate generator */
    PIT_OP_MODE_WAVE = 0x03 << 1,      /* square wave generator */
    PIT_OP_MODE_SW_STROBE = 0x04 << 1, /* software triggered strobe */
    PIT_OP_MODE_HW_STROBE = 0x05 << 1, /* hardware triggered strobe */
    PIT_OP_MODE_RATE_6 = 0x06 << 1,    /* rate generator */
    PIT_OP_MODE_WAVE_7 = 0x07 << 1     /* square wave generator */
};
typedef enum pit_operational_mode pit_operational_mode_t;

#define PIT_BCD_MODE 1

extern void init_pit(uint8_t dst_cpus);
extern void pit_disable(void);

#endif
