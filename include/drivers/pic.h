#ifndef KTF_DRV_PIC_H
#define KTF_DRV_PIC_H

#include <ktf.h>

#define PIC1_PORT_CMD 0x20
#define PIC2_PORT_CMD 0xa0
#define PIC1_PORT_DATA (PIC1_PORT_CMD + 1)
#define PIC2_PORT_DATA (PIC2_PORT_CMD + 1)

#define PIC_EOI 0x20 /* End-of-interrupt command code */

#define PIC_ICW1_ICW4        0x01
#define PIC_ICW1_SINGLE      0x02 /* Single (cascade) mode */
#define PIC_ICW1_INTERVAL4   0x04 /* Call address interval 4 (8) */
#define PIC_ICW1_LEVEL       0x08 /* Level triggered (edge) mode */
#define PIC_ICW1_INIT        0x10

#define PIC_ICW4_8086        0x01 /* 8086/88 (MCS-80/85) mode */
#define PIC_ICW4_AUTO        0x02 /* Auto (normal) EOI */
#define PIC_ICW4_BUF_PIC2    0x08 /* Buffered mode PIC2 */
#define PIC_ICW4_BUF_PIC1    0x0C /* Buffered mode PIC1 */
#define PIC_ICW4_SFNM        0x10 /* Special fully nested mode */

#define PIC_CASCADE_PIC2_IRQ 0x02
#define PIC_CASCADE_PIC1_IRQ 0x04

#define PIC_IRQ0_OFFSET      0x20
#define PIC_IRQ8_OFFSET      0x28

/* External declarations */

extern void init_pic(void);

#endif /* KTF_DRV_PIC_H */
