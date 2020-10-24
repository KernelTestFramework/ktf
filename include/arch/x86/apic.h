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

#define APIC_BASE_BSP    (_U64(1) << 8)
#define APIC_BASE_EXTD   (_U64(1) << 10)
#define APIC_BASE_ENABLE (_U64(1) << 11)

/* Local APIC definitions */
#define APIC_ID                0x020
#define APIC_LVR               0x030
#define APIC_EOI               0x0b0
#define APIC_SPIV              0x0f0
#define APIC_SPIV_APIC_ENABLED 0x00100

#define APIC_ICR        0x300
#define APIC_ICR2       0x310
#define APIC_LVT_TIMER  0x320
#define APIC_LVT_LINT0  0x350
#define APIC_LVT_LINT1  0x360
#define APIC_DM_NMI     0x00400
#define APIC_DM_INIT    0x00500
#define APIC_DM_STARTUP 0x00600
#define APIC_ICR_BUSY   0x01000
#define APIC_DISABLE    0x10000
#define APIC_DEST_SELF  0x40000

#define GET_APIC_DEST_FIELD(x) (((x) >> 24) & 0xFF)
#define SET_APIC_DEST_FIELD(x) ((x) << 24)

#define GET_SIPI_VECTOR(addr) ((_ul((addr)) >> PAGE_SHIFT) & 0xFF)

#define APIC_EOI_SIGNAL 0x0

#ifndef __ASSEMBLY__

enum apic_mode {
    APIC_MODE_UNKNOWN,
    APIC_MODE_NONE,
    APIC_MODE_DISABLED,
    APIC_MODE_XAPIC,
    APIC_MODE_X2APIC,
};
typedef enum apic_mode apic_mode_t;

union apic_base {
    struct {
        /* clang-format off */
        uint64_t rsvd0 : 8,
                 bsp   : 1,
                 rsvd1 : 1,
                 extd  : 1,
                 en    : 1,
                 base  : 24,
                 rsvd2 : 28;
        /* clang-format on */
    } __packed;
    uint64_t reg;
};
typedef union apic_base apic_base_t;

union apic_spiv {
    struct {
        /* clang-format off */
        uint32_t vector           : 8,
                 apic_enable      : 1,
                 focus_proc_check : 1,
                 rsvd0            : 2,
                 eoi_brdcast_supp : 1,
                 rsvd1            : 19;
        /* clang-format on */
    } __packed;
    uint32_t reg;
};
typedef union apic_spiv apic_spiv_t;

union apic_id {
    struct {
        /* clang-format off */
        uint32_t rsvd0         : 24,
                 old_xapic_id  : 4,
                 rsvd1         : 4;
        /* clang-format on */
    } __packed;
    struct {
        /* clang-format off */
        uint32_t rsvd2     : 24,
                 xapic_id  : 8;
        /* clang-format on */
    } __packed;
    uint32_t x2apic_id;
    uint32_t reg;
};
typedef union apic_id apic_id_t;

union apic_version {
    struct {
        /* clang-format off */
        uint32_t version          : 8,
                 rsvd0            : 8,
                 max_lvt_entry    : 8,
                 eoi_brdcast_supp : 1,
                 rsvd1            : 7;
        /* clang-format on */
    } __packed;
    uint32_t reg;
};
typedef union apic_version apic_version_t;

enum apic_trigger_mode {
    APIC_TRIGGER_MODE_EDGE = 0x00,
    APIC_TRIGGER_MODE_LEVEL = 0x01,
};
typedef enum apic_trigger_mode apic_trigger_mode_t;

enum apic_deliv_status {
    APIC_DELIV_STATUS_IDLE = 0x00,
    APIC_DELIV_STATUS_SEND_PENDING = 0x01,
};
typedef enum apic_deliv_status apic_deliv_status_t;

enum apic_lvt_mask {
    APIC_LVT_UNMASKED = 0x00,
    APIC_LVT_MASKED = 0x01,
};
typedef enum apic_lvt_mask apic_lvt_mask_t;

enum apic_lvt_deliv_mode {
    APIC_LVT_DELIV_MODE_FIXED = 0x00,
    APIC_LVT_DELIV_MODE_SMI = 0x02,
    APIC_LVT_DELIV_MODE_NMI = 0x04,
    APIC_LVT_DELIV_MODE_INIT = 0x05,
    APIC_LVT_DELIV_MODE_EXTINT = 0x07,
};
typedef enum apic_lvt_deliv_mode apic_lvt_deliv_mode_t;

enum apic_lvt_timer_mode {
    APIC_LVT_TIMER_ONE_SHOT = 0x00,
    APIC_LVT_TIMER_PERIODIC = 0x01,
    APIC_LVT_TIMER_TSC_DEADLINE = 0x10,
};
typedef enum apic_lvt_timer_mode apic_lvt_timer_mode_t;

union apic_lvt_timer {
    struct {
        /* clang-format off */
        uint32_t vector           : 8,
                 rsvd0            : 4,
                 deliv_status     : 1,
                 rsvd1            : 3,
                 mask             : 1,
                 timer_mode       : 2,
                 rsvd2            : 13;
        /* clang-format on */
    } __packed;
    uint32_t reg;
};
typedef union apic_lvt_timer apic_lvt_timer_t;

union apic_lvt_common {
    struct {
        /* clang-format off */
        uint32_t vector           : 8,
                 deliv_mode       : 3,
                 rsvd0            : 1,
                 deliv_status     : 1,
                 rsvd1            : 3,
                 mask             : 1,
                 rsvd2            : 15;
        /* clang-format on */
    } __packed;
    uint32_t reg;
};
typedef union apic_lvt_common apic_lvt_cmci_t;
typedef union apic_lvt_common apic_lvt_pm_counters_t;
typedef union apic_lvt_common apic_lvt_thermal_sensor_t;

union apic_lvt_lint {
    struct {
        /* clang-format off */
        uint32_t vector           : 8,
                 deliv_mode       : 3,
                 rsvd0            : 1,
                 deliv_status     : 1,
                 polarity         : 1,
                 remote_irr       : 1,
                 trigger_mode     : 1,
                 mask             : 1,
                 rsvd2            : 15;
        /* clang-format on */
    } __packed;
    uint32_t reg;
};
typedef union apic_lvt_lint apic_lvt_lint_t;

union apic_lvt_error {
    struct {
        /* clang-format off */
        uint32_t vector           : 8,
                 rsvd0            : 4,
                 deliv_status     : 1,
                 rsvd1            : 3,
                 mask             : 1,
                 rsvd2            : 15;
        /* clang-format on */
    } __packed;
    uint32_t reg;
};
typedef union apic_lvt_error apic_lvt_error_t;

union apic_esr {
    struct {
        /* clang-format off */
        uint32_t send_checksum_err : 1,
                 recv_checksum_err : 1,
                 send_accept_err   : 1,
                 recv_accept_err   : 1,
                 redirectable_ipi  : 1,
                 send_ill_vector   : 1,
                 recv_ill_vector   : 1,
                 ill_reg_address   : 1,
                 rsvd0             : 24;
        /* clang-format on */
    } __packed;
    uint32_t reg;
};
typedef union apic_esr apic_esr_t;

enum apic_timer_divide {
    APIC_TIMER_DIVIDE_BY_2 = 0x00,
    APIC_TIMER_DIVIDE_BY_4 = 0x08,
    APIC_TIMER_DIVIDE_BY_8 = 0x02,
    APIC_TIMER_DIVIDE_BY_16 = 0x0a,
    APIC_TIMER_DIVIDE_BY_32 = 0x01,
    APIC_TIMER_DIVIDE_BY_64 = 0x09,
    APIC_TIMER_DIVIDE_BY_128 = 0x03,
    APIC_TIMER_DIVIDE_BY_1 = 0x0b,
};
typedef enum apic_timer_divide apic_timer_divide_t;

union apic_timer_div_config {
    struct {
        /* clang-format off */
        uint32_t divide_value : 4,
                 rsvd0        : 28;
        /* clang-format on */
    } __packed;
    uint32_t reg;
};
typedef union apic_timer_div_config apic_timer_div_config_t;

enum apic_deliv_mode {
    APIC_DELIV_MODE_FIXED = 0x00,
    APIC_DELIV_MODE_LOWPRI = 0x01,
    APIC_DELIV_MODE_SMI = 0x02,
    APIC_DELIV_MODE_NMI = 0x04,
    APIC_DELIV_MODE_INIT = 0x05,
    APIC_DELIV_MODE_SIPI = 0x06,
};
typedef enum apic_deliv_mode apic_deliv_mode_t;

enum apic_dest_mode {
    APIC_DEST_MODE_PHYSICAL = 0x00,
    APIC_DEST_MODE_LOGICAL = 0x01,
};
typedef enum apic_dest_mode apic_dest_mode_t;

enum apic_icr_level {
    APIC_ICR_LEVEL_DEASSERT = 0x00,
    APIC_ICR_LEVEL_ASSERT = 0x01,
};
typedef enum apic_icr_level apic_icr_level_t;

enum apic_dest_shorthand {
    APIC_DEST_SHORTHAND_NONE = 0x00,
    APIC_DEST_SHORTHAND_SELF = 0x01,
    APIC_DEST_SHORTHAND_ALL = 0x10,
    APIC_DEST_SHORTHAND_OTHERS = 0x11,
};
typedef enum apic_dest_shorthand apic_dest_shorthand_t;

union apic_icr {
    struct {
        /* clang-format off */
        uint64_t vector           : 8,
                 deliv_mode       : 3,
                 dest_mode        : 1,
                 deliv_status     : 1,
                 rsvd0            : 1,
                 level            : 1,
                 trigger_mode     : 1,
                 rsvd1            : 2,
                 dest_shorthand   : 2,
                 rsvd2            : 36,
                 xapic_dest       : 8;
        /* clang-format on */
    } __packed;
    struct {
        uint32_t rsvd3;
        uint32_t x2apic_dest;
    } __packed;
    struct {
        uint32_t icr0;
        uint32_t icr1;
    } __packed;
    uint64_t reg;
};
typedef union apic_icr apic_icr_t;

union apic_ldr {
    struct {
        /* clang-format off */
        uint32_t rsvd0    : 24,
                 xapic_id  : 8;
        /* clang-format on */
    } __packed;
    uint32_t x2apic_id;
    uint32_t reg;
};
typedef union apic_ldr apic_ldr_t;

union apic_dfr {
    struct {
        /* clang-format off */
        uint32_t rsvd0   : 28,
                 model   : 4;
        /* clang-format on */
    } __packed;
    uint32_t reg;
};
typedef union apic_dfr apic_dfr_t;

union apic_priority_class {
    struct {
        /* clang-format off */
        uint32_t sub_class : 4,
                 class     : 4,
                 rsvd0     : 24;
        /* clang-format on */
    } __packed;
    uint32_t reg;
};
typedef union apic_priority_class apic_apr_t;
typedef union apic_priority_class apic_tpr_t;
typedef union apic_priority_class apic_ppr_t;

union apic_request_register {
    struct {
        uint16_t rsvd0;
        uint8_t bytes[30];
    } __packed;
    struct {
        uint64_t rsvd1 : 16, qword0 : 48;
        uint64_t qword1;
        uint64_t qword2;
        uint64_t qword3;
    } __packed;
    uint64_t reg[4];
};
typedef union apic_request_register apic_irr_t;
typedef union apic_request_register apic_isr_t;
typedef union apic_request_register apic_tmr_t;

union apic_self_ipi {
    struct {
        /* clang-format off */
        uint32_t vector           : 8,
                 rsvd0            : 24;
        /* clang-format on */
    } __packed;
    uint32_t reg;
};
typedef union apic_self_ipi apic_self_ipi_t;

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
    apic_mmio_write(APIC_ICR2, (uint32_t)(val >> 32));
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
        apic_mmio_write(APIC_ICR2, (uint32_t)(val >> 32));
        apic_mmio_write(APIC_ICR, (uint32_t) val);
    }
    else
        apic_msr_write(MSR_X2APIC_REGS + (APIC_ICR >> 4), val);
}

static inline void apic_wait_ready(void) {
    while (apic_read(APIC_ICR) & APIC_ICR_BUSY)
        cpu_relax();
}

static inline void apic_EOI(void) { apic_write(APIC_EOI, APIC_EOI_SIGNAL); }

#endif /* __ASSEMBLY__ */

#endif /* KTF_APIC_H */
