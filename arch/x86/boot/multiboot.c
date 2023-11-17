/*
 * Copyright © 2020 Amazon.com, Inc. or its affiliates.
 * Copyright © 2022 Open Source Security, Inc.
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
#include <acpi_ktf.h>
#include <console.h>
#include <drivers/fb.h>
#include <errno.h>
#include <mm/pmm.h>
#include <multiboot2.h>
#include <pagetable.h>

#define TAG_ADDR(tag)                                                                    \
    ((multiboot2_tag_t *) ((multiboot2_uint8_t *) (tag) + (((tag)->size + 7) & ~7)))
#define TAG_STRING(tag) (((struct multiboot2_tag_string *) (tag))->string)

static void *multiboot2_hdr;
static size_t multiboot2_hdr_size;

static multiboot2_memory_map_t *multiboot_mmap;
static unsigned multiboot_mmap_num;

static unsigned multiboot_mem_lower;
static unsigned multiboot_mem_upper;

static const char *multiboot_region_type_name[] = {
    [MULTIBOOT2_MEMORY_AVAILABLE] = "Available",
    [MULTIBOOT2_MEMORY_RESERVED] = "Reserved",
    [MULTIBOOT2_MEMORY_ACPI_RECLAIMABLE] = "ACPI Reclaimable",
    [MULTIBOOT2_MEMORY_NVS] = "NVS",
};

static void handle_multiboot_mmap(struct multiboot2_tag_mmap *tag) {
    multiboot2_uint8_t *tag_end = (multiboot2_uint8_t *) tag + tag->size;

    BUG_ON(sizeof(multiboot2_memory_map_t) != tag->entry_size);

    multiboot_mmap = tag->entries;

    for (multiboot2_memory_map_t *mmap = tag->entries; _ptr(mmap) < _ptr(tag_end); mmap++)
        multiboot_mmap_num++;
}

void display_multiboot_mmap(void) {
    printk("Physical Memory Map\n");

    for (unsigned int i = 0; i < multiboot_mmap_num; i++) {
        multiboot2_memory_map_t *mmap = &multiboot_mmap[i];

        printk("REGION: [0x%016lx - 0x%016lx] %s\n", mmap->addr, mmap->addr + mmap->len,
               multiboot_region_type_name[mmap->type]);
    }
}

void init_multiboot(unsigned long *addr, const char **cmdline) {
    dprintk("Initialize multiboot2\n");

    multiboot2_hdr = addr;
    multiboot2_hdr_size = *addr++;

    dprintk("[multiboot2] header at %p of size: %lx\n", multiboot2_hdr,
            multiboot2_hdr_size);

    for (multiboot2_tag_t *tag = (multiboot2_tag_t *) addr;
         tag->type != MULTIBOOT2_TAG_TYPE_END; tag = TAG_ADDR(tag)) {
        switch (tag->type) {
        case MULTIBOOT2_TAG_TYPE_CMDLINE:
            *cmdline = TAG_STRING(tag);
            break;

        case MULTIBOOT2_TAG_TYPE_BOOT_LOADER_NAME:
            printk("[multiboot2] Boot loader name: %s\n", TAG_STRING(tag));
            break;

        case MULTIBOOT2_TAG_TYPE_MODULE: {
            struct multiboot2_tag_module *mod = (struct multiboot2_tag_module *) tag;

            printk("[multiboot2] Module: [0x%08x - 0x%08x], Command line %s\n",
                   mod->mod_start, mod->mod_end, mod->cmdline);
        } break;

        case MULTIBOOT2_TAG_TYPE_BASIC_MEMINFO: {
            struct multiboot2_tag_basic_meminfo *meminfo =
                (struct multiboot2_tag_basic_meminfo *) tag;

            multiboot_mem_lower = meminfo->mem_lower;
            multiboot_mem_upper = meminfo->mem_upper;

            printk("[multiboot2] Lower memory: %u KB, Upper memory: %u KB\n",
                   multiboot_mem_lower, multiboot_mem_upper);
        } break;

        case MULTIBOOT2_TAG_TYPE_BOOTDEV: {
            struct multiboot2_tag_bootdev *bootdev =
                (struct multiboot2_tag_bootdev *) tag;

            printk("[multiboot2] Boot device 0x%x,%x,%x\n", bootdev->biosdev,
                   bootdev->slice, bootdev->part);
        } break;

        case MULTIBOOT2_TAG_TYPE_MMAP:
            handle_multiboot_mmap((struct multiboot2_tag_mmap *) tag);
            break;

        case MULTIBOOT2_TAG_TYPE_FRAMEBUFFER: {
            struct multiboot2_tag_framebuffer *fb =
                (struct multiboot2_tag_framebuffer *) tag;

            switch (fb->common.framebuffer_type) {
            case MULTIBOOT2_FRAMEBUFFER_TYPE_INDEXED:
            case MULTIBOOT2_FRAMEBUFFER_TYPE_RGB:
                init_framebuffer(fb);
                break;
            case MULTIBOOT2_FRAMEBUFFER_TYPE_EGA_TEXT:
            default:
                printk("[multiboot2] Unsupported framebuffer type: %u\n",
                       fb->common.framebuffer_type);
                break;
            }
        } break;

        case MULTIBOOT2_TAG_TYPE_EFI32: {
            struct multiboot2_tag_efi32 *efi32 = (struct multiboot2_tag_efi32 *) tag;

            printk("[multiboot2] EFI32 Pointer: 0x%x\n", efi32->pointer);
        } break;

        case MULTIBOOT2_TAG_TYPE_EFI64: {
            struct multiboot2_tag_efi64 *efi64 = (struct multiboot2_tag_efi64 *) tag;

            printk("[multiboot2] EFI64 Pointer: 0x%lx\n", efi64->pointer);
        } break;

        case MULTIBOOT2_TAG_TYPE_ACPI_OLD: {
            struct multiboot2_tag_old_acpi *acpi = (struct multiboot2_tag_old_acpi *) tag;

            acpi_rsdp = _paddr(&acpi->rsdp);

            printk("[multiboot2] ACPI RSDPv1: 0x%016lx\n", acpi_rsdp);
        } break;

        case MULTIBOOT2_TAG_TYPE_ACPI_NEW: {
            struct multiboot2_tag_new_acpi *acpi = (struct multiboot2_tag_new_acpi *) tag;

            acpi_rsdp = _paddr(&acpi->rsdp);

            printk("[multiboot2] ACPI RSDPv2: 0x%016lx\n", acpi_rsdp);
        } break;

        case MULTIBOOT2_TAG_TYPE_LOAD_BASE_ADDR: {
            struct multiboot2_tag_load_base_addr *addr =
                (struct multiboot2_tag_load_base_addr *) tag;

            printk("[multiboot2] Load base address: 0x%x\n", addr->load_base_addr);
        } break;

        default:
            printk("Tag 0x%02x, Size 0x%04x\n", tag->type, tag->size);
            break;
        }
    }
}

void map_multiboot_areas(void) {
    paddr_t mbi_start = _paddr(multiboot2_hdr);
    paddr_t mbi_stop = mbi_start + multiboot2_hdr_size;

    for (mfn_t mfn = paddr_to_mfn(mbi_start); mfn <= paddr_to_mfn(mbi_stop); mfn++) {
        vmap_kern_4k(mfn_to_virt(mfn), mfn, L1_PROT_RO);
        vmap_kern_4k(mfn_to_virt_kern(mfn), mfn, L1_PROT_RO);
    }
}

unsigned mbi_get_avail_memory_ranges_num(void) {
    unsigned num = 0;

    for (unsigned i = 0; i < multiboot_mmap_num; i++) {
        multiboot2_memory_map_t *entry = &multiboot_mmap[i];

        if (entry->type == MULTIBOOT2_MEMORY_AVAILABLE)
            num++;
    }

    if (multiboot_mmap_num == 0) {
        if (multiboot_mem_lower > 0)
            num++;
        if (multiboot_mem_upper > 0)
            num++;
    }

    return num;
}

int mbi_get_avail_memory_range(unsigned index, addr_range_t *r) {
    unsigned avail = 0;

    for (unsigned int i = 0; i < multiboot_mmap_num; i++) {
        multiboot2_memory_map_t *entry = &multiboot_mmap[i];

        if (entry->type != MULTIBOOT2_MEMORY_AVAILABLE)
            continue;

        if (avail++ == index) {
            r->start = _ptr(_paddr(entry->addr));
            r->end = _ptr(_paddr(r->start + entry->len));
            return 0;
        }
    }

    if (multiboot_mmap_num == 0) {
        r->start = _ptr(index == 0 ? 0x0 : MB(1));
        r->end = r->start + (index == 0 ? multiboot_mem_lower : multiboot_mem_upper);
        return r->end > r->start ? 0 : -ENOENT;
    }

    return -ENOMEM;
}

int mbi_get_memory_range(paddr_t pa, addr_range_t *r) {
    paddr_t _start, _end;

    for (unsigned int i = 0; i < multiboot_mmap_num; i++) {
        multiboot2_memory_map_t *entry = &multiboot_mmap[i];

        _start = _paddr(entry->addr);
        _end = _paddr(_start + entry->len);

        if (pa >= _start && pa < _end)
            goto found;
    }

    if (multiboot_mmap_num == 0) {
        _start = 0x0;
        _end = multiboot_mem_lower;

        if (pa < _end)
            goto found;

        _start = MB(1);
        _end = _start + multiboot_mem_upper;

        if (pa >= _start && pa < _end)
            goto found;
    }

    return -ENOMEM;

found:
    if (r) {
        r->start = _ptr(_start);
        r->end = _ptr(_end);
    }

    return 0;
}
