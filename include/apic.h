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
#ifndef KTF_APIC_H
#define KTF_APIC_H

#include <ktf.h>
#include <lib.h>
#include <processor.h>

#define DEFAULT_APIC_BASE _U32(0xfee00000)

#define APIC_BASE_BSP     (_U64(1) << 8)
#define APIC_BASE_EXTD    (_U64(1) << 10)
#define APIC_BASE_ENABLE  (_U64(1) << 11)

/* Local APIC definitions */
#define APIC_ID                0x020
#define APIC_LVR               0x030
#define APIC_SPIV              0x0f0
#define APIC_SPIV_APIC_ENABLED 0x00100

#define APIC_ICR               0x300
#define APIC_ICR2              0x310
#define APIC_LVT_TIMER         0x320
#define APIC_LVT_LINT0         0x350
#define APIC_LVT_LINT1         0x360
#define APIC_DM_NMI            0x00400
#define APIC_DM_INIT           0x00500
#define APIC_DM_STARTUP        0x00600
#define APIC_ICR_BUSY          0x01000
#define APIC_DISABLE           0x10000
#define APIC_DEST_SELF         0x40000

#define GET_APIC_DEST_FIELD(x) (((x) >> 24) & 0xFF)
#define SET_APIC_DEST_FIELD(x) ((x) << 24)

#define GET_SIPI_VECTOR(addr) ((_ul((addr)) >> PAGE_SHIFT) & 0xFF)

#ifndef __ASSEMBLY__

enum apic_mode {
    APIC_MODE_UNKNOWN,
    APIC_MODE_NONE,
    APIC_MODE_DISABLED,
    APIC_MODE_XAPIC,
    APIC_MODE_X2APIC,
};
typedef enum apic_mode apic_mode_t;

/* External declarations */

extern apic_mode_t apic_mode;

extern int init_apic(enum apic_mode mode);

/* Static declarations */

/* XAPIC Mode */
static inline uint32_t apic_mmio_read(uint32_t reg) {
    return *(volatile uint32_t *) (_ptr(DEFAULT_APIC_BASE) + reg);
}

static inline void apic_mmio_write(uint32_t reg, uint32_t val) {
    *(volatile uint32_t *) (_ptr(DEFAULT_APIC_BASE) + reg) = val;
}

static inline void apic_mmio_icr_write(uint64_t val) {
    apic_mmio_write(APIC_ICR2, (uint32_t) (val >> 32));
    apic_mmio_write(APIC_ICR, (uint32_t) val);
}

/* X2APIC Mode */
static inline uint32_t apic_msr_read(uint32_t reg) {
    return (uint32_t) rdmsr(MSR_X2APIC_REGS + (reg >> 4));
}

static inline void apic_msr_write(uint32_t reg, uint32_t val) {
    wrmsr(MSR_X2APIC_REGS + (reg >> 4), _ul(val));
}

static inline void apic_write(uint32_t reg, uint32_t val) {
    if (apic_mode == APIC_MODE_XAPIC)
        apic_mmio_write(reg, val);
    else
        apic_msr_write(reg, val);
}

static inline uint32_t apic_read(uint32_t reg) {
    if (apic_mode == APIC_MODE_XAPIC)
        return apic_mmio_read(reg);

    return apic_msr_read(reg);
}

static inline void apic_icr_write(uint64_t val) {
    if (apic_mode == APIC_MODE_XAPIC) {
        apic_mmio_write(APIC_ICR2, (uint32_t) (val >> 32));
        apic_mmio_write(APIC_ICR, (uint32_t) val);
    }
    else
        apic_msr_write(MSR_X2APIC_REGS + (APIC_ICR >> 4), val);
}

static inline void apic_wait_ready(void) {
    while (apic_read(APIC_ICR) & APIC_ICR_BUSY)
        cpu_relax();
}

#endif /* __ASSEMBLY__ */

#endif /* KTF_APIC_H */
