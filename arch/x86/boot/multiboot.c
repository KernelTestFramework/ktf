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

unsigned mbi_get_avail_memory_ranges_num(void) {
    unsigned num = 0;

    if (has_mbi_flag(MULTIBOOT_INFO_MEM_MAP)) {
        for (int i = 0; i < multiboot_mmap_num; i++) {
            multiboot_memory_map_t *entry = &multiboot_mmap[i];

            if (entry->type == MULTIBOOT_MEMORY_AVAILABLE)
                num++;
        }
    }
    else if (has_mbi_flag(MULTIBOOT_INFO_MEMORY))
        num = 2;

    return num;
}

int mbi_get_avail_memory_range(unsigned index, addr_range_t *r) {
    unsigned avail = 0;

    if (has_mbi_flag(MULTIBOOT_INFO_MEM_MAP)) {
        for (int i = 0; i < multiboot_mmap_num; i++) {
            multiboot_memory_map_t *entry = &multiboot_mmap[i];

            if (entry->type != MULTIBOOT_MEMORY_AVAILABLE)
                continue;

            if (avail++ == index) {
                r->start = _ptr(_paddr(entry->addr));
                r->end = _ptr(_paddr(r->start + entry->len));
                return 0;
            }

        }
    }
    else if (has_mbi_flag(MULTIBOOT_INFO_MEMORY)) {
        if (index == 0) {
            r->start = 0x0;
            r->end = _ptr(multiboot_info->mem_lower * KB(1));
            return 0;
        }
        else if (index == 1) {
            r->start = _ptr(MB(1));
            r->end = r->start + (multiboot_info->mem_upper * KB(1));
            return 0;
        }
    }

    return -1;
}

int mbi_get_memory_range(paddr_t pa, addr_range_t *r) {
    paddr_t _start, _end;

    if (has_mbi_flag(MULTIBOOT_INFO_MEM_MAP)) {
        for (int i = 0; i < multiboot_mmap_num; i++) {
            multiboot_memory_map_t *entry = &multiboot_mmap[i];

            _start = _paddr(entry->addr);
            _end = _paddr(_start + entry->len);

            if (pa >= _start && pa < _end)
                goto found;
        }
    }
    else if (has_mbi_flag(MULTIBOOT_INFO_MEMORY)) {
        _start = 0x0;
        _end = multiboot_info->mem_lower * KB(1);

        if (pa >= _start && pa < _end)
            goto found;

        _start = MB(1);
        _end = _start + multiboot_info->mem_upper * KB(1);

        if (pa >= _start && pa < _end)
            goto found;
    }

    return -1;

found:
    r->start = _ptr(_start);
    r->end = _ptr(_end);

    return 0;
}
