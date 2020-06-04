#include <ktf.h>
#include <string.h>
#include <console.h>
#include <multiboot.h>

static multiboot_info_t *multiboot_info;

static multiboot_memory_map_t *multiboot_mmap;
static unsigned multiboot_mmap_num;

static const char *multiboot_region_type_name[] = {
    [MULTIBOOT_MEMORY_UNDEFINED]        = "Undefined",
    [MULTIBOOT_MEMORY_AVAILABLE]        = "Available",
    [MULTIBOOT_MEMORY_RESERVED]         = "Reserved",
    [MULTIBOOT_MEMORY_ACPI_RECLAIMABLE] = "ACPI Reclaimable",
    [MULTIBOOT_MEMORY_NVS]              = "NVS",
    [MULTIBOOT_MEMORY_BADRAM]           = "Bad RAM",
};

static inline bool has_mbi_flag(unsigned flag) {
    return multiboot_info && !!(multiboot_info->flags & flag);
}

void display_multiboot_mmap(void) {
    if (!has_mbi_flag(MULTIBOOT_INFO_MEMORY))
        return;

    printk("\nPhysical Memory Map\n");

    if (!has_mbi_flag(MULTIBOOT_INFO_MEM_MAP)) {
        printk("REGION: [0x%016lx - 0x%016lx] Lower memory\n",
               0, multiboot_info->mem_lower * KB(1));
        printk("REGION: [0x%016lx - 0x%016lx] Upper memory\n",
               MB(1), MB(1) + (multiboot_info->mem_upper * KB(1)));
        return;
    }

    for (int i = 0; i < multiboot_mmap_num; i++) {
        multiboot_memory_map_t *entry = &multiboot_mmap[i];

        if (entry->type != MULTIBOOT_MEMORY_UNDEFINED) {
            printk("REGION: [0x%016lx - 0x%016lx] %s\n",
                   entry->addr, entry->addr + entry->len,
                   multiboot_region_type_name[entry->type]);
        }
    }
}

void init_multiboot(multiboot_info_t *mbi, const char **cmdline) {
    multiboot_info = mbi;

    if (has_mbi_flag(MULTIBOOT_INFO_MEM_MAP)) {
        multiboot_mmap = (multiboot_memory_map_t *) _ptr(mbi->mmap_addr);
        multiboot_mmap_num = mbi->mmap_length / sizeof(*multiboot_mmap);
    }

    if (has_mbi_flag(MULTIBOOT_INFO_CMDLINE) && cmdline)
        *cmdline = (const char *) _ptr(mbi->cmdline);
}

uint32_t mbi_lower_memory(void) {
    return multiboot_info->mem_lower;
}

uint32_t mbi_upper_memory(void) {
    return multiboot_info->mem_upper;
}
