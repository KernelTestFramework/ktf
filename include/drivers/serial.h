/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef KTF_DRV_SERIAL_H
#define KTF_DRV_SERIAL_H

#include <ktf.h>

union line_control_register {
    uint8_t reg;
    struct __packed {
        uint8_t width:2,
                stop_bit:1,
                parity:3,
                break_ctrl:1,
                DLAB:1;
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
        uint8_t dtr:1,
                rts:1,
                aux:2,
                lo:1,
                auto_flow:1,
                rsvd:2;
    };
};
typedef union modem_control_register mcr_t;

union line_status_register {
    uint8_t reg;
    struct __packed {
        uint8_t data_avl:1,
                overrun_err:1,
                parity_err:1,
                framing_err:1,
                break_sig:1,
                thr_empty:1,
                thr_empty_idle:1,
                fifo_err:1;
    };
};
typedef union line_status_register lsr_t;

union modem_status_register {
    uint8_t reg;
    struct __packed {
        uint8_t cts_change:1,
                dsr_change:1,
                ri_change:1,
                cd_change:1,
                cts:1,
                dsr:1,
                ri:1,
                cd:1;
    };
};
typedef union modem_status_register msr_t;

union fifo_control_register {
    uint8_t reg;
    struct __packed {
        uint8_t enable:1,
                clear_rx:1,
                clear_tx:1,
                dma_mode:1,
                rsvd:1,
                enable_64:1,
                int_lvl:2;
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
        uint8_t rx_avl:1,
                thr_empty:1,
                rx_lsr_change:1,
                msr_change:1,
                sleep_mode: 1,
                low_pwr:1,
                rsvd:2;
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
extern int serial_putchar(io_port_t port, char c);
extern int serial_write(io_port_t port, const char *buf, size_t len);

#endif /* KTF_DRV_SERIAL_H */
