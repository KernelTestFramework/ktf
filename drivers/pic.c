#include <ktf.h>
#include <lib.h>
#include <drivers/pic.h>

static inline void pic_outb(io_port_t port, unsigned char value) {
    outb(port, value);
    io_delay();
}

void init_pic(void) {
    /* Cascade mode initialization sequence */
    pic_outb(PIC1_PORT_CMD, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    pic_outb(PIC2_PORT_CMD, PIC_ICW1_INIT | PIC_ICW1_ICW4);

    /* Remap PICs interrupt vectors */
    pic_outb(PIC1_PORT_DATA, PIC_IRQ0_OFFSET);
    pic_outb(PIC2_PORT_DATA, PIC_IRQ8_OFFSET);

    /* Set PIC1 and PIC2 cascade IRQ */
    outb(PIC1_PORT_DATA, PIC_CASCADE_PIC1_IRQ);
    outb(PIC2_PORT_DATA, PIC_CASCADE_PIC2_IRQ);

    /* PIC mode: 80x86, Automatic EOI */
    outb(PIC1_PORT_DATA, PIC_ICW4_8086 | PIC_ICW4_AUTO);
    outb(PIC2_PORT_DATA, PIC_ICW4_8086 | PIC_ICW4_AUTO);

    /* Mask the 8259A PICs by setting all IMR bits */
    outb(PIC1_PORT_DATA, 0xFF);
    outb(PIC2_PORT_DATA, 0xFF);
}
