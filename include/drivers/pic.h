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
#ifndef KTF_DRV_PIC_H
#define KTF_DRV_PIC_H

#include <ktf.h>

#define PIC1_PORT_CMD  0x20
#define PIC2_PORT_CMD  0xa0
#define PIC1_PORT_DATA (PIC1_PORT_CMD + 1)
#define PIC2_PORT_DATA (PIC2_PORT_CMD + 1)

#define PIC_EOI 0x20 /* End-of-interrupt command code */

#define PIC_ICW1_ICW4      0x01
#define PIC_ICW1_SINGLE    0x02 /* Single (cascade) mode */
#define PIC_ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define PIC_ICW1_LEVEL     0x08 /* Level triggered (edge) mode */
#define PIC_ICW1_INIT      0x10

#define PIC_ICW4_8086     0x01 /* 8086/88 (MCS-80/85) mode */
#define PIC_ICW4_AUTO     0x02 /* Auto (normal) EOI */
#define PIC_ICW4_BUF_PIC2 0x08 /* Buffered mode PIC2 */
#define PIC_ICW4_BUF_PIC1 0x0C /* Buffered mode PIC1 */
#define PIC_ICW4_SFNM     0x10 /* Special fully nested mode */

#define PIC_CASCADE_PIC2_IRQ 0x02
#define PIC_CASCADE_PIC1_IRQ 0x04

#define PIC_IRQ0_OFFSET 0x20
#define PIC_IRQ8_OFFSET 0x28

#define PIC_IRQ_END_OFFSET 0x08 /* One beyond the max IRQ number */

enum pic_device_sel { PIC1_DEVICE_SEL = 1, PIC2_DEVICE_SEL };
typedef enum pic_device_sel pic_device_sel_t;

/* External declarations */

extern void init_pic(void);
extern void pic_enable_irq(pic_device_sel_t pic, uint8_t irq);

#endif /* KTF_DRV_PIC_H */
