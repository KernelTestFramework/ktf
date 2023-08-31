/*
 * Copyright © 2020 Amazon.com, Inc. or its affiliates.
 * Copyright © 2014,2015 Citrix Systems Ltd.
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
#ifndef KTF_TRAPS_H
#define KTF_TRAPS_H

#include <processor.h>

#define EXC_BASE 0
#define IRQ_BASE 32

#define SYSCALL_INT 0x80

#define MAX_INT 256

#ifndef __ASSEMBLY__
#include <cpu.h>
#include <drivers/keyboard.h>
#include <drivers/pit.h>
#include <drivers/serial.h>

#define SERIAL_COM1_IRQ COM1_IRQ_VECTOR
#define SERIAL_COM2_IRQ COM2_IRQ_VECTOR
#define TIMER_IRQ       PIT_IRQ_VECTOR
#define KB_PORT1_IRQ    KEYBOARD_PORT1_IRQ_VECTOR
#define KB_PORT2_IRQ    KEYBOARD_PORT2_IRQ_VECTOR
#define APIC_TIMER_IRQ  APIC_TIMER_IRQ_VECTOR

extern void init_traps(const cpu_t *cpu);
extern void init_boot_traps(void);
extern void init_rmode_traps(void);

extern void print_callstack(const void *sp, const void *ip);
extern void do_exception(struct cpu_regs *regs);

#endif /* __ASSEMBLY__ */

#endif /* KTF_TRAPS_H */
