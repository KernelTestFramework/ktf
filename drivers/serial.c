#include <ktf.h>
#include <lib.h>
#include <setup.h>

#include <drivers/serial.h>

static inline void set_port_mode(io_port_t port, bool stop_bit, uint8_t width) {
    lcr_t lcr = { 0 };

    lcr.stop_bit = stop_bit;
    lcr.width= width;
    outb(port + UART_LCR_REG_OFFSET, lcr.reg);
}

static inline void set_dlab(io_port_t port, bool dlab) {
    lcr_t lcr;

    lcr.reg = inb(port + UART_LCR_REG_OFFSET);
    lcr.DLAB = dlab;
    outb(port + UART_LCR_REG_OFFSET, lcr.reg);
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

void uart_init(io_port_t port, unsigned baud) {
    mcr_t mcr = { 0 };

    /* Disable interrupts */
    outb(port + UART_IER_REG_OFFSET, 0x00);

    /* Disable FIFO control */
    outb(port + UART_FCR_REG_OFFSET, 0x00);

    /* Set 8n1 mode */
    set_port_mode(port, STOP_BIT_1, FRAME_SIZE_8_BITS);

    /* Set baud speed by applying divisor to DLL+DLH */
    set_dlab(port, true);
    outw(port + UART_DLL_REG_OFFSET, DEFAULT_BAUD_SPEED / baud);
    set_dlab(port, false);

    /* Set tx/rx ready state */
    mcr.dtr = 1;
    mcr.rts = 1;
    outb(port + UART_MCR_REG_OFFSET, mcr.reg);
}

static inline int uart_port_status(io_port_t port) {
    if (!receiver_ready(port))
        return -1; /* BUSY */

    if (!thr_empty(port))
        return 1; /* EAGAIN */

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

#define SERIAL_RETRIES 100000
int serial_putchar(io_port_t port, char c) {
   unsigned retries = SERIAL_RETRIES;
   int rc;

   do {
       rc = uart_putc(port, c);
   } while(rc > 0 && retries--);

   return rc;
}

int serial_write(io_port_t port, const char *buf, size_t len) {
   unsigned retries = SERIAL_RETRIES;
   int rc;

   do {
       rc = uart_puts(port, buf, len);
   } while(rc > 0 && retries--);

   return rc;
}
