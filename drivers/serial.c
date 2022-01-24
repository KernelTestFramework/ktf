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
#include <ioapic.h>
#include <ktf.h>
#include <lib.h>
#include <setup.h>

#include <drivers/serial.h>

#define INPUT_BUF 80
static struct {
    char buf[INPUT_BUF];
    unsigned curr;
    unsigned init;
} input_state;

io_port_t __data_rmode com_ports[4];

static inline const char *com_port_name(com_port_t port) {
    switch (port) {
    case COM1_PORT:
        return "COM1";
    case COM2_PORT:
        return "COM2";
    case COM3_PORT:
        return "COM3";
    case COM4_PORT:
        return "COM4";
    default:
        BUG();
    }
}

static const uint8_t com_frame_size_values[] = {
    [COM_FRAME_SIZE_5_BITS] = 5,
    [COM_FRAME_SIZE_6_BITS] = 6,
    [COM_FRAME_SIZE_7_BITS] = 7,
    [COM_FRAME_SIZE_8_BITS] = 8,
};

static const char com_parity_names[] = {
    [COM_NO_PARITY] = 'n',   [COM_ODD_PARITY] = 'o', [COM_EVEN_PARITY] = 'e',
    [COM_HIGH_PARITY] = 'h', [COM_LOW_PARITY] = 'l',
};

#define COM_STOP_BIT_VALUE(cfg) ((cfg)->stop_bit + 1)

void display_uart_config(const uart_config_t *cfg) {
    printk("[%s] 0x%x %u,%u%c%u\n", com_port_name(cfg->port), cfg->port, cfg->baud,
           com_frame_size_values[cfg->frame_size], com_parity_names[cfg->parity],
           COM_STOP_BIT_VALUE(cfg));
}

io_port_t get_first_com_port(void) {
    memcpy((void *) com_ports, _ptr(BDA_COM_PORTS_ENTRY), sizeof(com_ports));

    for (unsigned int i = 0; i < ARRAY_SIZE(com_ports); i++) {
        if (com_ports[i] != 0x0)
            return com_ports[i];
    }

    /* Fallback to COM1 */
    return COM1_PORT;
}

static inline void set_dlab(io_port_t port, bool dlab) {
    lcr_t lcr;

    lcr.reg = inb(port + UART_LCR_REG_OFFSET);
    lcr.DLAB = dlab;
    outb(port + UART_LCR_REG_OFFSET, lcr.reg);
}

static inline void set_port_mode(uart_config_t *cfg) {
    lcr_t lcr = {0};

    lcr.stop_bit = cfg->stop_bit;
    lcr.width = cfg->frame_size;
    lcr.parity = cfg->parity;

    outb(cfg->port + UART_LCR_REG_OFFSET, lcr.reg);

    /* Set baud speed by applying divisor to DLL+DLH */
    set_dlab(cfg->port, true);
    outw(cfg->port + UART_DLL_REG_OFFSET, DEFAULT_BAUD_SPEED / cfg->baud);
    set_dlab(cfg->port, false);
}

static inline bool thr_empty(io_port_t port) {
    lsr_t lsr;

    lsr.reg = inb(port + UART_LSR_REG_OFFSET);

    return lsr.thr_empty;
}

static inline bool receiver_ready(io_port_t port) {
    msr_t msr;

    msr.reg = inb(port + UART_MSR_REG_OFFSET);

    return msr.dsr && msr.cts;
}

void __text_init init_uart(uart_config_t *cfg) {
    mcr_t mcr = {0};

    /* Enable interrupts for received data available */
    outb(cfg->port + UART_IER_REG_OFFSET, 0x01);

    /* Disable FIFO control */
    outb(cfg->port + UART_FCR_REG_OFFSET, 0x00);

    /* Set 8n1 mode */
    set_port_mode(cfg);

    /* Set tx/rx ready state */
    mcr.dtr = 1;
    mcr.rts = 1;
    outb(cfg->port + UART_MCR_REG_OFFSET, mcr.reg);
}

void __text_init init_uart_input(uint8_t dst_cpus) {
    /* Initialize input state */
    memset(&input_state, 0, sizeof(input_state));

    /* Enable IRQ lines */
    printk("Enabling serial input\n");

    configure_isa_irq(COM1_IRQ, COM1_IRQ0_OFFSET, IOAPIC_DEST_MODE_PHYSICAL, dst_cpus);
    configure_isa_irq(COM2_IRQ, COM2_IRQ0_OFFSET, IOAPIC_DEST_MODE_PHYSICAL, dst_cpus);
}

static inline int uart_port_status(io_port_t port) {
    if (!receiver_ready(port))
        return -1; /* ENODEV */

    if (!thr_empty(port)) {
        io_delay();
        return 1; /* EAGAIN */
    }

    return 0;
}

static int uart_putc(io_port_t port, char c) {
    int rc = uart_port_status(port);

    if (rc == 0)
        putc(c, port + UART_TXD_REG_OFFSET);

    return rc;
}

static int uart_puts(io_port_t port, const char *buf, size_t len) {
    int rc = uart_port_status(port);

    if (rc == 0)
        puts(port + UART_TXD_REG_OFFSET, buf, len);

    return rc;
}

#define SERIAL_TIMEOUT 1000 /* ~1s */
int serial_putchar(io_port_t port, char c) {
    unsigned retries = SERIAL_TIMEOUT;
    int rc;

    do {
        rc = uart_putc(port, c);
        BUG_ON(rc < 0);
    } while (rc > 0 && retries--);

    return rc;
}

int serial_write(io_port_t port, const char *buf, size_t len) {
    unsigned retries = SERIAL_TIMEOUT;
    int rc;

    do {
        rc = uart_puts(port, buf, len);
        BUG_ON(rc < 0);
    } while (rc > 0 && retries--);

    return rc;
}

void uart_interrupt_handler(void) {
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(com_ports); ++i) {
        uint8_t status = inb(com_ports[i] + UART_IIR_REG_OFFSET);
        if ((status & UART_IIR_STATUS_MASK) == UART_IIR_RBR_READY) {
            uint8_t input = inb(com_ports[i] + UART_RBR_REG_OFFSET);

            input_state.buf[input_state.curr] = input;
            input_state.curr = (input_state.curr + 1) % sizeof(input_state.buf);

            printk("%c", input);
        }
    }

    apic_EOI();
}
