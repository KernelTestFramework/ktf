#include <ktf.h>
#include <lib.h>
#include <setup.h>
#include <console.h>
#include <multiboot.h>

static multiboot_info_t multiboot_info;
#define MAX_MULTIBOOT_MMAP_ENTRIES 16
static multiboot_memory_map_t multiboot_mmap[MAX_MULTIBOOT_MMAP_ENTRIES];

static const char *multiboot_region_type_name[] = {
    [MULTIBOOT_MEMORY_UNDEFINED]        = "Undefined",
    [MULTIBOOT_MEMORY_AVAILABLE]        = "Available",
    [MULTIBOOT_MEMORY_RESERVED]         = "Reserved",
    [MULTIBOOT_MEMORY_ACPI_RECLAIMABLE] = "ACPI Reclaimable",
    [MULTIBOOT_MEMORY_NVS]              = "NVS",
    [MULTIBOOT_MEMORY_BADRAM]           = "Bad RAM",
};

void display_multiboot_mmap(void) {
    printk("\nPhysical Memory Map\n");

    for (int i = 0; i < ARRAY_SIZE(multiboot_mmap); i++) {
        multiboot_memory_map_t *entry = &multiboot_mmap[i];

        if (entry->type != MULTIBOOT_MEMORY_UNDEFINED) {
            printk("REGION: [0x%016lx - 0x%016lx] %s\n",
                   entry->addr, entry->addr + entry->len,
                   multiboot_region_type_name[entry->type]);
        }
    }
}

void init_multiboot(multiboot_info_t *mbi) {
    memcpy(&multiboot_info, mbi, sizeof(multiboot_info));
    memcpy(&multiboot_mmap, _ptr(mbi->mmap_addr), mbi->mmap_length);
    strcpy(kernel_cmdline, _ptr(mbi->cmdline));
}
