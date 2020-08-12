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
#ifndef KTF_DRV_SERIAL_H
#define KTF_DRV_SERIAL_H

#include <ktf.h>

union line_control_register {
    uint8_t reg;
    struct __packed {
        uint8_t width : 2, stop_bit : 1, parity : 3, break_ctrl : 1, DLAB : 1;
    };
};
typedef union line_control_register lcr_t;

#define FRAME_SIZE_8_BITS 0x03
#define FRAME_SIZE_7_BITS 0x02
#define FRAME_SIZE_6_BITS 0x01
#define FRAME_SIZE_5_BITS 0x00

#define STOP_BIT_1 0x00
#define STOP_BIT_2 0x01

#define NO_PARITY   0x00
#define ODD_PARITY  0x01
#define EVEN_PARITY 0x03
#define HIGH_PARITY 0x05
#define LOW_PARITY  0x07

union modem_control_register {
    uint8_t reg;
    struct __packed {
        uint8_t dtr : 1, rts : 1, aux : 2, lo : 1, auto_flow : 1, rsvd : 2;
    };
};
typedef union modem_control_register mcr_t;

union line_status_register {
    uint8_t reg;
    struct __packed {
        uint8_t data_avl : 1, overrun_err : 1, parity_err : 1, framing_err : 1,
            break_sig : 1, thr_empty : 1, thr_empty_idle : 1, fifo_err : 1;
    };
};
typedef union line_status_register lsr_t;

union modem_status_register {
    uint8_t reg;
    struct __packed {
        uint8_t cts_change : 1, dsr_change : 1, ri_change : 1, cd_change : 1, cts : 1,
            dsr : 1, ri : 1, cd : 1;
    };
};
typedef union modem_status_register msr_t;

union fifo_control_register {
    uint8_t reg;
    struct __packed {
        uint8_t enable : 1, clear_rx : 1, clear_tx : 1, dma_mode : 1, rsvd : 1,
            enable_64 : 1, int_lvl : 2;
    };
};
typedef union fifo_control_register fcr_t;

#define FIFO_INT_TRIGGER_LEVEL_1  0x00
#define FIFO_INT_TRIGGER_LEVEL_4  0x01
#define FIFO_INT_TRIGGER_LEVEL_8  0x10
#define FIFO_INT_TRIGGER_LEVEL_14 0x11

union interrupt_enable_register {
    uint8_t reg;
    struct __packed {
        uint8_t rx_avl : 1, thr_empty : 1, rx_lsr_change : 1, msr_change : 1,
            sleep_mode : 1, low_pwr : 1, rsvd : 2;
    };
};
typedef union interrupt_enable_register ier_t;

#define UART_TXD_REG_OFFSET 0x00
#define UART_IER_REG_OFFSET 0x01
#define UART_DLL_REG_OFFSET 0x00
#define UART_DLH_REG_OFFSET 0x01
#define UART_FCR_REG_OFFSET 0x02
#define UART_LCR_REG_OFFSET 0x03
#define UART_MCR_REG_OFFSET 0x04
#define UART_LSR_REG_OFFSET 0x05
#define UART_MSR_REG_OFFSET 0x06
#define UART_SCR_REG_OFFSET 0x07

#define DEFAULT_BAUD_SPEED 115200

/* External declarations */

extern void uart_init(io_port_t port, unsigned baud);
extern int  serial_putchar(io_port_t port, char c);
extern int  serial_write(io_port_t port, const char *buf, size_t len);

#endif /* KTF_DRV_SERIAL_H */
