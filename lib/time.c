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
#include <percpu.h>
#include <time.h>

static __aligned(16) volatile time_t ticks = 0;

void timer_interrupt_handler(void) {
    asm volatile("lock incq %[ticks]" : [ ticks ] "=m"(ACCESS_ONCE(ticks)));
    apic_EOI();
}

void apic_timer_interrupt_handler(void) {
    asm volatile("lock incq %%gs:%[ticks]"
                 : [ ticks ] "=m"(ACCESS_ONCE(PERCPU_VAR(apic_ticks))));
    apic_EOI();
}

void msleep(time_t ms) {
    time_t end;

    end = ACCESS_ONCE(ticks) + ms;
    while (ACCESS_ONCE(ticks) < end)
        cpu_relax();
}

void msleep_local(time_t ms) {
    time_t end = PERCPU_GET(apic_ticks) + ms;
    while (PERCPU_GET(apic_ticks) < end) {
        cpu_relax();
    }
}

time_t get_timer_ticks(void) {
    return ACCESS_ONCE(ticks);
}

time_t get_local_ticks(void) {
    return PERCPU_GET(apic_ticks);
}
