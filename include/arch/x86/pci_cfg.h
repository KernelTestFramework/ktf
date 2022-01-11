/*
 * Copyright © 2021 Amazon.com, Inc. or its affiliates.
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
#ifndef KTF_PCI_CFG_H
#define KTF_PCI_CFG_H

#include <ktf.h>
#include <lib.h>
#include <spinlock.h>

#define PCI_IO_PORT_ADDRESS 0xcf8
#define PCI_IO_PORT_DATA    0xcfc

extern spinlock_t pci_cfg_lock;

union pci_cfg_addr {
    struct {
        uint32_t zero : 2, reg : 6, fn : 3, dev : 5, bus : 8, rsvd : 7, ecsm : 1;
    } __packed;
    uint32_t val;
};
typedef union pci_cfg_addr pci_cfg_addr_t;

union pci_cfg_value {
    uint8_t byte[4];
    uint16_t word[2];
    uint32_t dword;
};
typedef union pci_cfg_value pci_cfg_value_t;

/* Static declarations */

static inline void pci_cfg_set_addr(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg) {
    pci_cfg_addr_t addr;

    addr.val = 0;
    addr.reg = reg;
    addr.fn = func;
    addr.dev = dev;
    addr.bus = bus;
    addr.ecsm = 1;

    outd(PCI_IO_PORT_ADDRESS, addr.val);
}

static inline uint8_t pci_cfg_read8(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg) {
    uint8_t ret;

    spin_lock(&pci_cfg_lock);

    pci_cfg_set_addr(bus, dev, func, reg);
    ret = inb(PCI_IO_PORT_DATA);

    spin_unlock(&pci_cfg_lock);

    return ret;
}

static inline void pci_cfg_write8(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg,
                                  uint8_t value) {
    spin_lock(&pci_cfg_lock);

    pci_cfg_set_addr(bus, dev, func, reg);
    outb(PCI_IO_PORT_DATA, value);

    spin_unlock(&pci_cfg_lock);
}

static inline uint16_t pci_cfg_read16(uint8_t bus, uint8_t dev, uint8_t func,
                                      uint8_t reg) {
    uint16_t ret;

    spin_lock(&pci_cfg_lock);

    pci_cfg_set_addr(bus, dev, func, reg);
    ret = inw(PCI_IO_PORT_DATA);

    spin_unlock(&pci_cfg_lock);

    return ret;
}

static inline void pci_cfg_write16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg,
                                   uint16_t value) {
    spin_lock(&pci_cfg_lock);

    pci_cfg_set_addr(bus, dev, func, reg);
    outw(PCI_IO_PORT_DATA, value);

    spin_unlock(&pci_cfg_lock);
}

static inline uint32_t pci_cfg_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg) {
    uint32_t ret;

    spin_lock(&pci_cfg_lock);

    pci_cfg_set_addr(bus, dev, func, reg);
    ret = ind(PCI_IO_PORT_DATA);

    spin_unlock(&pci_cfg_lock);

    return ret;
}

static inline void pci_cfg_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg,
                                 uint32_t value) {
    spin_lock(&pci_cfg_lock);

    pci_cfg_set_addr(bus, dev, func, reg);
    outd(PCI_IO_PORT_DATA, value);

    spin_unlock(&pci_cfg_lock);
}

/* External declarations */

#endif /* KTF_PCI_CFG_H */
