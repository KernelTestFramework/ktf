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
#ifdef KTF_ACPICA
#include <ktf.h>
#include <mm/slab.h>
#include <time.h>

#include "acpi.h"

/* General OS functions */

ACPI_STATUS AcpiOsInitialize(void) {
    dprintk("ACPI OS Initialization:\n");

    return AE_OK;
}

ACPI_STATUS AcpiOsTerminate(void) {
    dprintk("ACPI OS Termination:\n");

    return AE_OK;
}

ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info) {
    switch (Function) {
    case ACPI_SIGNAL_FATAL: {
        ACPI_SIGNAL_FATAL_INFO *info = Info;

        panic("ACPI: Received ACPI_SIGNAL_FATAL: Type: %u, Code: %u, Arg: %u",
              info ? info->Type : 0, info ? info->Code : 0, info ? info->Argument : 0);
    } break;
    case ACPI_SIGNAL_BREAKPOINT: {
        char *bp_msg = Info;

        printk("ACPI: Received ACPI_SIGNAL_BREAKPOINT: %s", bp_msg ?: "");
    } break;
    default:
        BUG();
    }

    return AE_OK;
}

ACPI_STATUS AcpiOsEnterSleep(UINT8 SleepState, UINT32 RegaValue, UINT32 RegbValue) {
    dprintk("ACPI Entering sleep state S%u.\n", SleepState);

    return AE_OK;
}

/* Memory and IO space read/write functions */

ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 *Value, UINT32 Width) {
    void *pa = _ptr(_paddr(Address));
    UINT64 val = 0;

    switch (Width) {
    case 8:
        val = *(uint8_t *) pa;
        break;
    case 16:
        val = *(uint16_t *) pa;
        break;
    case 32:
        val = *(uint32_t *) pa;
        break;
    case 64:
        val = *(uint64_t *) pa;
        break;
    default:
        return AE_BAD_PARAMETER;
    }

    *Value = val;
    return AE_OK;
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width) {
    void *pa = _ptr(_paddr(Address));

    switch (Width) {
    case 8:
        *(uint8_t *) pa = (uint8_t) Value;
        break;
    case 16:
        *(uint16_t *) pa = (uint16_t) Value;
        break;
    case 32:
        *(uint32_t *) pa = (uint32_t) Value;
        break;
    case 64:
        *(uint64_t *) pa = (uint64_t) Value;
        break;
    default:
        return AE_BAD_PARAMETER;
    }

    return AE_OK;
}

ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width) {
    switch (Width) {
    case 8:
        *Value = inb(Address);
        break;
    case 16:
        *Value = inw(Address);
        break;
    case 32:
        *Value = ind(Address);
        break;
    default:
        return AE_BAD_PARAMETER;
    }

    return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width) {
    switch (Width) {
    case 8:
        outb(Address, (uint8_t) Value);
        break;
    case 16:
        outw(Address, (uint16_t) Value);
        break;
    case 32:
        outd(Address, (uint32_t) Value);
        break;
    default:
        return AE_BAD_PARAMETER;
    }

    return AE_OK;
}

/* General Table handling functions */

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void) {
    ACPI_PHYSICAL_ADDRESS pa = 0;

    AcpiFindRootPointer(&pa);
    return pa;
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *PredefinedObject,
                                     ACPI_STRING *NewValue) {
    if (!NewValue)
        return AE_BAD_PARAMETER;

    *NewValue = NULL;
    return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable,
                                ACPI_TABLE_HEADER **NewTable) {
    if (!NewTable)
        return AE_BAD_PARAMETER;

    *NewTable = NULL;
    return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable,
                                        ACPI_PHYSICAL_ADDRESS *NewAddress,
                                        UINT32 *NewTableLength) {
    if (!NewAddress || !NewTableLength)
        return AE_BAD_PARAMETER;

    *NewAddress = _paddr(NULL);
    *NewTableLength = 0;

    return AE_OK;
}

/* Memory management functions */

void *AcpiOsAllocate(ACPI_SIZE Size) { return kmalloc(Size); }

void AcpiOsFree(void *Memory) { kfree(Memory); }

/* FIXME: Check if pages are mapped (with exception tables) */
BOOLEAN AcpiOsReadable(void *Memory, ACPI_SIZE Length) { return true; }

/* FIXME: Check if pages are mapped and writeable (with exception tables) */
BOOLEAN AcpiOsWriteable(void *Memory, ACPI_SIZE Length) { return true; }

void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length) {
    unsigned offset = PhysicalAddress & ~PAGE_MASK;
    unsigned num_pages = ((offset + Length) / PAGE_SIZE) + 1;
    mfn_t mfn = paddr_to_mfn(PhysicalAddress);
    void *va = NULL;

    for (unsigned i = 0; i < num_pages; i++, mfn++) {
        void *_va = kmap_4k(mfn, L1_PROT);
        if (!_va)
            return NULL;

        if (!va)
            va = _ptr(_ul(_va) + offset);
    }

    return va;
}

void AcpiOsUnmapMemory(void *LogicalAddress, ACPI_SIZE Length) {
    unsigned offset = _ul(LogicalAddress) & ~PAGE_MASK;
    unsigned num_pages = ((offset + Length) / PAGE_SIZE) + 1;
    mfn_t mfn = virt_to_mfn(LogicalAddress);

    for (unsigned i = 0; i < num_pages; i++, mfn++)
        kunmap(mfn_to_virt_kern(mfn), PAGE_ORDER_4K);
}

/* Time management functions */

void AcpiOsSleep(UINT64 Miliseconds) { msleep(Miliseconds); }

/* FIXME: Return in correct 100ns units */
UINT64 AcpiOsGetTimer(void) { return get_timer_ticks(); }

/* FIXME: Use microseconds granularity */
void AcpiOsStall(UINT32 Microseconds) { msleep(1); }
#endif /* KTF_ACPICA */
