#include <ktf.h>
#include <string.h>
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

static inline bool has_mbi_flag(unsigned flag) {
    return !!(multiboot_info.flags & flag);
}

void display_multiboot_mmap(void) {
    if (!has_mbi_flag(MULTIBOOT_INFO_MEMORY))
        return;

    printk("\nPhysical Memory Map\n");
    printk("REGION: Lower %8u KB\n", multiboot_info.mem_lower);
    printk("REGION: Upper %8u KB\n", multiboot_info.mem_upper);

    if (!has_mbi_flag(MULTIBOOT_INFO_MEM_MAP))
        return;

    for (int i = 0; i < ARRAY_SIZE(multiboot_mmap); i++) {
        multiboot_memory_map_t *entry = &multiboot_mmap[i];

        if (entry->type != MULTIBOOT_MEMORY_UNDEFINED) {
            printk("REGION: [0x%016lx - 0x%016lx] %s\n",
                   entry->addr, entry->addr + entry->len,
                   multiboot_region_type_name[entry->type]);
        }
    }
}

void init_multiboot(multiboot_info_t *mbi, const char **cmdline) {
    memcpy(&multiboot_info, mbi, sizeof(multiboot_info));
    if (has_mbi_flag(MULTIBOOT_INFO_MEM_MAP))
        memcpy(&multiboot_mmap, _ptr(mbi->mmap_addr), mbi->mmap_length);
    if (has_mbi_flag(MULTIBOOT_INFO_CMDLINE))
        strcpy(kernel_cmdline, _ptr(mbi->cmdline));
}

uint32_t mbi_lower_memory(void) {
    return multiboot_info.mem_lower;
}

uint32_t mbi_upper_memory(void) {
    return multiboot_info.mem_upper;
}
