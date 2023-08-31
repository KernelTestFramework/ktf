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

#include <cpu.h>
#include <drivers/pic.h>
#include <ktf.h>

enum com_idx {
    COM1 = 0,
    COM2 = 1,
    COM3 = 2,
    COM4 = 3,
};
typedef enum com_idx com_idx_t;

enum com_port {
    NO_COM_PORT = 0x0,
    COM1_PORT = 0x3f8,
    COM2_PORT = 0x2f8,
    COM3_PORT = 0x3e8,
    COM4_PORT = 0x2e8,
};
typedef enum com_port com_port_t;

enum com_baud {
    COM_BAUD_300 = 300,
    COM_BAUD_1200 = 1200,
    COM_BAUD_2400 = 2400,
    COM_BAUD_4800 = 4800,
    COM_BAUD_9600 = 9600,
    COM_BAUD_19200 = 19200,
    COM_BAUD_38400 = 38400,
    COM_BAUD_57600 = 57600,
    COM_BAUD_115200 = 115200,
};
typedef enum com_baud com_baud_t;

#define DEFAULT_BAUD_SPEED COM_BAUD_115200

enum com_frame_size {
    COM_FRAME_SIZE_8_BITS = 0x03,
    COM_FRAME_SIZE_7_BITS = 0x02,
    COM_FRAME_SIZE_6_BITS = 0x01,
    COM_FRAME_SIZE_5_BITS = 0x00,
};
typedef enum com_frame_size com_frame_size_t;

enum com_stop_bit {
    COM_STOP_BIT_1 = 0x00,
    COM_STOP_BIT_2 = 0x01,
};
typedef enum com_stop_bit com_stop_bit_t;

enum com_parity {
    COM_NO_PARITY = 0x00,
    COM_ODD_PARITY = 0x01,
    COM_EVEN_PARITY = 0x03,
    COM_HIGH_PARITY = 0x05,
    COM_LOW_PARITY = 0x07,
};
typedef enum com_parity com_parity_t;

struct uart_config {
    io_port_t port;
    com_baud_t baud;
    com_frame_size_t frame_size;
    com_parity_t parity;
    com_stop_bit_t stop_bit;
};
typedef struct uart_config uart_config_t;

union line_control_register {
    uint8_t reg;
    struct __packed {
        uint8_t width : 2, stop_bit : 1, parity : 3, break_ctrl : 1, DLAB : 1;
    };
};
typedef union line_control_register lcr_t;

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

union interrupt_identification_register {
    uint8_t reg;
    struct __packed {
        uint8_t no_int_pend : 1, event : 3, rsvd : 1, fifo_64b : 1, fifo_status : 2;
    };
};
typedef union interrupt_identification_register iir_t;

#define UART_IIR_EVENT_MSR_CHANGE   0x0
#define UART_IIR_EVENT_THR_EMPTY    0x1
#define UART_IIR_EVENT_RXD_AVAIL    0x2
#define UART_IIR_EVENT_LSR_CHANGE   0x3
#define UART_IIR_EVENT_CHAR_TIMEOUT 0x6

#define UART_IIR_FIFO_NO_FIFO       0x0
#define UART_IIR_FIFO_UNUSABLE_FIFO 0x1
#define UART_IIR_FIFO_ENABLED       0x3

enum com_irq {
    COM1_IRQ = 4, /* IRQ 4 */
    COM2_IRQ = 3, /* IRQ 3 */
    COM3_IRQ = COM1_IRQ,
    COM4_IRQ = COM2_IRQ,
};
typedef enum com_irq com_irq_t;

#define COM1_IRQ_OFFSET (PIC_IRQ0_OFFSET + COM1_IRQ)
#define COM1_IRQ_VECTOR COM1_IRQ_OFFSET
#define COM2_IRQ_OFFSET (PIC_IRQ0_OFFSET + COM2_IRQ)
#define COM2_IRQ_VECTOR COM2_IRQ_OFFSET

#define UART_TXD_REG_OFFSET 0x00
#define UART_RBR_REG_OFFSET 0x00
#define UART_IER_REG_OFFSET 0x01
#define UART_DLL_REG_OFFSET 0x00
#define UART_DLH_REG_OFFSET 0x01
#define UART_IIR_REG_OFFSET 0x02
#define UART_FCR_REG_OFFSET 0x02
#define UART_LCR_REG_OFFSET 0x03
#define UART_MCR_REG_OFFSET 0x04
#define UART_LSR_REG_OFFSET 0x05
#define UART_MSR_REG_OFFSET 0x06
#define UART_SCR_REG_OFFSET 0x07

/* External declarations */

extern io_port_t com_ports[4];

extern io_port_t get_first_com_port(void);
extern void init_uart(uart_config_t *cfg);
extern void init_uart_input(const cpu_t *cpu);
extern void uart_interrupt_handler(void);
extern int serial_putchar(io_port_t port, char c);
extern int serial_write(io_port_t port, const char *buf, size_t len);

extern void display_uart_config(const uart_config_t *cfg);

#endif /* KTF_DRV_SERIAL_H */
