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
#include <drivers/pic.h>
#include <ktf.h>
#include <lib.h>

static inline void pic_outb(io_port_t port, unsigned char value) {
    outb(port, value);
    io_delay();
}

void init_pic(void) {
    /* Cascade mode initialization sequence */
    pic_outb(PIC1_PORT_CMD, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    pic_outb(PIC2_PORT_CMD, PIC_ICW1_INIT | PIC_ICW1_ICW4);

    /* Remap PICs interrupt vectors */
    pic_outb(PIC1_PORT_DATA, PIC_IRQ0_OFFSET);
    pic_outb(PIC2_PORT_DATA, PIC_IRQ8_OFFSET);

    /* Set PIC1 and PIC2 cascade IRQ */
    outb(PIC1_PORT_DATA, PIC_CASCADE_PIC1_IRQ);
    outb(PIC2_PORT_DATA, PIC_CASCADE_PIC2_IRQ);

    /* PIC mode: 80x86, Automatic EOI */
    outb(PIC1_PORT_DATA, PIC_ICW4_8086 | PIC_ICW4_AUTO);
    outb(PIC2_PORT_DATA, PIC_ICW4_8086 | PIC_ICW4_AUTO);

    /* Mask the 8259A PICs by setting all IMR bits */
    outb(PIC1_PORT_DATA, 0xFF);
    outb(PIC2_PORT_DATA, 0xFF);
}
