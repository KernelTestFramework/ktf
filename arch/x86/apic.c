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
#include <console.h>
#include <ktf.h>
#include <lib.h>
#include <percpu.h>
#include <processor.h>

static apic_mode_t apic_mode = APIC_MODE_UNKNOWN;

static const char *apic_mode_names[] = {
    /* clang-format off */
    [APIC_MODE_UNKNOWN]  = "Unknown",
    [APIC_MODE_NONE]     = "None",
    [APIC_MODE_DISABLED] = "Disabled",
    [APIC_MODE_XAPIC]    = "XAPIC",
    [APIC_MODE_X2APIC]   = "X2APIC",
    /* clang-format on */
};

/* XAPIC Mode */
static inline uint32_t apic_mmio_read(xapic_regs_t reg) {
    return *(volatile uint32_t *) (apic_get_base(PERCPU_GET(apic_base)) + reg);
}

static inline void apic_mmio_write(xapic_regs_t reg, uint32_t val) {
    *(volatile uint32_t *) (apic_get_base(PERCPU_GET(apic_base)) + reg) = val;
}

/* X2APIC Mode */
static inline uint64_t apic_msr_read(x2apic_regs_t reg) {
    return rdmsr(MSR_X2APIC_REGS + (reg >> 4));
}

static inline void apic_msr_write(x2apic_regs_t reg, uint64_t val) {
    wrmsr(MSR_X2APIC_REGS + (reg >> 4), val);
}

uint64_t apic_read(unsigned int reg) {
    if (apic_mode == APIC_MODE_XAPIC)
        return apic_mmio_read(reg);
    else if (apic_mode == APIC_MODE_X2APIC)
        return apic_msr_read(reg);
    else
        BUG();
    UNREACHABLE();
}

void apic_write(unsigned int reg, uint64_t val) {
    if (apic_mode == APIC_MODE_XAPIC)
        apic_mmio_write(XAPIC_REG(reg), (uint32_t) val);
    else if (apic_mode == APIC_MODE_X2APIC)
        apic_msr_write(X2APIC_REG(reg), val);
    else
        BUG();
}

void apic_icr_write(uint64_t val) {
    if (apic_mode == APIC_MODE_XAPIC) {
        apic_mmio_write(APIC_ICR1, (uint32_t)(val >> 32));
        apic_mmio_write(APIC_ICR0, (uint32_t) val);
    }
    else
        apic_msr_write(MSR_X2APIC_REGS + (APIC_ICR0 >> 4), val);
}

apic_mode_t apic_get_mode(void) { return apic_mode; }

void init_apic(unsigned int cpu, apic_mode_t mode) {
    percpu_t *percpu = get_percpu_page(cpu);
    apic_base_t apic_base;

    BUG_ON(mode < APIC_MODE_DISABLED);

    apic_base.reg = rdmsr(MSR_APIC_BASE);

    /* Boot up initialization */
    if (apic_mode == APIC_MODE_UNKNOWN) {
        if (apic_base.en && apic_base.extd)
            apic_mode = APIC_MODE_X2APIC;
        else if (apic_base.en && !apic_base.extd)
            apic_mode = APIC_MODE_XAPIC;
        else if (!apic_base.en && !apic_base.extd)
            apic_mode = APIC_MODE_DISABLED;
        else
            BUG();
    }

    printk("CPU%u: Initializing APIC mode: %s -> %s\n", cpu, apic_mode_names[apic_mode],
           apic_mode_names[mode]);

    /* Disable APIC */
    apic_base.en = apic_base.extd = 0;
    wrmsr(MSR_APIC_BASE, apic_base.reg);

    if (mode >= APIC_MODE_XAPIC) {
        apic_base.en = 1;
        wrmsr(MSR_APIC_BASE, apic_base.reg);
    }

    if (mode == APIC_MODE_X2APIC) {
        apic_base.extd = 1;
        wrmsr(MSR_APIC_BASE, apic_base.reg);
    }

    apic_mode = mode;
    percpu->apic_base = apic_base;
    PERCPU_SET(bsp, apic_base.bsp);

    /* XAPIC requires MMIO accesses, thus the APIC_BASE page needs to be mapped.
     * X2APIC uses MSRs for accesses, so no mapping needed.
     */
    if (apic_mode == APIC_MODE_XAPIC)
        vmap(apic_get_base(apic_base), apic_base.base, PAGE_ORDER_4K, L1_PROT);

    apic_write(APIC_SPIV, APIC_SPIV_APIC_ENABLED | 0xff);
}
