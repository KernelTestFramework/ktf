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
#include <atomic.h>
#include <drivers/pic.h>
#include <drivers/pit.h>
#include <lib.h>
#include <time.h>

static volatile uint64_t ticks = 0;

void init_pit(void) {
    outb(PIT_COMMAND_PORT,
         PIT_CHANNEL_0 & PIT_ACCESS_MODE_LH & PIT_OP_MODE_RATE & PIT_BCD_MODE);
    outb(PIT_DATA_PORT_CH0, PIT_FREQUENCY & 0xFF);          /* send low byte */
    outb(PIT_DATA_PORT_CH0, (PIT_FREQUENCY & 0xFF00) >> 8); /* send high byte */
    pic_enable_irq(PIC1_DEVICE_SEL, PIT_IRQ);
}

void pit_interrupt_handler(void) { ++ticks; }

void pit_sleep(uint64_t ms) {
    uint64_t end = ticks + ms;
    while (ticks < end) {
        cpu_relax();
    }
}
