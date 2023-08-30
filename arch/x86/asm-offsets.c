/*
 * Copyright © 2023 Open Source Security, Inc.
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

#include <compiler.h>
#include <percpu.h>

#define EMIT_DEFINE(s, v)                                                                \
    asm volatile(".ascii \"@@#define " #s " %0 /* " #v " */@@\"\n" ::"i"(v))
#define OFFSETOF(__symbol, __type, __member)                                             \
    EMIT_DEFINE(__symbol, offsetof(__type, __member));

void __asm_offset_header(void) {
    OFFSETOF(usermode_private, percpu_t, usermode_private);

    OFFSETOF(cpu_irq_ip, cpu_irq_t, _ASM_IP);
    OFFSETOF(cpu_irq_cs, cpu_irq_t, cs);
    OFFSETOF(cpu_irq_flags, cpu_irq_t, _ASM_FLAGS);
    OFFSETOF(cpu_irq_sp, cpu_irq_t, _ASM_SP);
    OFFSETOF(cpu_irq_ss, cpu_irq_t, ss);

    OFFSETOF(cpu_exc_error_code, cpu_exc_t, error_code);
    OFFSETOF(cpu_exc_vector, cpu_exc_t, vector);
    OFFSETOF(cpu_exc_ip, cpu_exc_t, irq._ASM_IP);
    OFFSETOF(cpu_exc_cs, cpu_exc_t, irq.cs);
    OFFSETOF(cpu_exc_flags, cpu_exc_t, irq._ASM_FLAGS);
    OFFSETOF(cpu_exc_sp, cpu_exc_t, irq._ASM_SP);
    OFFSETOF(cpu_exc_ss, cpu_exc_t, irq.ss);

    OFFSETOF(cpu_regs_ax, cpu_regs_t, _ASM_AX);
    OFFSETOF(cpu_regs_bx, cpu_regs_t, _ASM_BX);
    OFFSETOF(cpu_regs_cx, cpu_regs_t, _ASM_CX);
    OFFSETOF(cpu_regs_dx, cpu_regs_t, _ASM_DX);
    OFFSETOF(cpu_regs_si, cpu_regs_t, _ASM_SI);
    OFFSETOF(cpu_regs_di, cpu_regs_t, _ASM_DI);
    OFFSETOF(cpu_regs_bp, cpu_regs_t, _ASM_BP);
#if defined(__x86_64__)
    OFFSETOF(cpu_regs_r8, cpu_regs_t, r8);
    OFFSETOF(cpu_regs_r9, cpu_regs_t, r9);
    OFFSETOF(cpu_regs_r10, cpu_regs_t, r10);
    OFFSETOF(cpu_regs_r11, cpu_regs_t, r11);
    OFFSETOF(cpu_regs_r12, cpu_regs_t, r12);
    OFFSETOF(cpu_regs_r13, cpu_regs_t, r13);
    OFFSETOF(cpu_regs_r14, cpu_regs_t, r14);
    OFFSETOF(cpu_regs_r15, cpu_regs_t, r15);
#endif

    OFFSETOF(cpu_regs_error_code, cpu_regs_t, exc.error_code);
    OFFSETOF(cpu_regs_vector, cpu_regs_t, exc.vector);
    OFFSETOF(cpu_regs_ip, cpu_regs_t, exc.irq._ASM_IP);
    OFFSETOF(cpu_regs_cs, cpu_regs_t, exc.irq.cs);
    OFFSETOF(cpu_regs_flags, cpu_regs_t, exc.irq._ASM_FLAGS);
    OFFSETOF(cpu_regs_sp, cpu_regs_t, exc.irq._ASM_SP);
    OFFSETOF(cpu_regs_ss, cpu_regs_t, exc.irq.ss);
}
