/*
 * Copyright © 2022 Amazon.com, Inc. or its affiliates.
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
#ifndef KTF_PCI_H
#define KTF_PCI_H

#include <list.h>

#define PCI_NR_BUS  256
#define PCI_NR_DEV  32
#define PCI_NR_FUNC 8

#define PCI_VENDOR_MASK    0x0000FFFF
#define PCI_VENDOR_INVALID 0xFFFF

#define PCI_DEVICE_MASK  0xFFFF0000
#define PCI_DEVICE_SHIFT 16

#define PCI_COMMAND_MASK 0x0000FFFF

#define PCI_STATUS_MASK     0xFFFF0000
#define PCI_STATUS_SHIFT    16
#define PCI_STATUS_CAP_LIST (1U << 4)

#define PCI_CLASS_MASK   0xFF000000
#define PCI_CLASS_SHIFT  24
#define PCI_CLASS_BRIDGE 0x06

#define PCI_SUBCLASS_MASK        0x00FF0000
#define PCI_SUBCLASS_SHIFT       16
#define PCI_SUBCLASS_HOST_BRIDGE 0x0

#define PCI_HDR_MASK            0x00FF0000
#define PCI_HDR_SHIFT           16
#define PCI_HDR_TYPE            0x7F
#define PCI_HDR_TYPE_NORMAL     0x00
#define PCI_HDR_TYPE_PCI_BRIDGE 0x01
#define PCI_HDR_MULTIFUNC       0x80

#define PCI_SEC_BUS_MASK  0x0000FF00
#define PCI_SEC_BUS_SHIFT 8
#define PCI_SUB_BUS_MASK  0x00FF0000
#define PCI_SUB_BUS_SHIFT 16

#define PCI_REG_VENDOR  0x0
#define PCI_REG_COMMAND 0x1
#define PCI_REG_CLASS   0x2
#define PCI_REG_HDR     0x3
#define PCI_REG_BUS     0x6
#define PCI_REG_CAP_PTR 0xD

#define PCI_VENDOR(cfg_reg0)   (cfg_reg0 & PCI_VENDOR_MASK)
#define PCI_DEVICE(cfg_reg0)   ((cfg_reg0 & PCI_DEVICE_MASK) >> PCI_DEVICE_SHIFT)
#define PCI_COMMAND(cfg_reg1)  (cfg_reg1 & PCI_COMMAND_MASK)
#define PCI_STATUS(cfg_reg1)   ((cfg_reg1 & PCI_STATUS_MASK) >> PCI_STATUS_SHIFT)
#define PCI_CLASS(cfg_reg2)    ((cfg_reg2 & PCI_CLASS_MASK) >> PCI_CLASS_SHIFT)
#define PCI_SUBCLASS(cfg_reg2) ((cfg_reg2 & PCI_SUBCLASS_MASK) >> PCI_SUBCLASS_SHIFT)
#define PCI_HDR(cfg_reg3)      ((cfg_reg3 & PCI_HDR_MASK) >> PCI_HDR_SHIFT)
#define PCI_SEC_BUS(cfg_reg6)  ((cfg_reg6 & PCI_SEC_BUS_MASK) >> PCI_SEC_BUS_SHIFT)
#define PCI_SUB_BUS(cfg_reg6)  ((cfg_reg6 & PCI_SUB_BUS_MASK) >> PCI_SUB_BUS_SHIFT)

#define PCI_DEV_EXISTS(cfg_reg0) (PCI_VENDOR(cfg_reg0) != PCI_VENDOR_INVALID)

struct pcidev {
    list_head_t list;
    uint32_t segment : 16;
    uint32_t bus : 8;
    uint32_t dev : 5;
    uint32_t func : 3;
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t subclass;
    uint8_t class;
    uint8_t hdr;
    uint8_t cap_ptr;
    struct pcidev *bridge;
    char bdf_str[10];
};
typedef struct pcidev pcidev_t;

extern void init_pci(void);

#endif /* KTF_PCI_H */
