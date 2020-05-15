#include <ktf.h>
#include <lib.h>
#include <page.h>
#include <traps.h>
#include <string.h>
#include <setup.h>
#include <segment.h>
#include <console.h>
#include <pagetable.h>
#include <multiboot.h>

/*
 * KTF Stack layout:
 *
 * kernel_stack[page 5] Emergency stack
 * kernel_stack[page 4] Exception stack
 * kernel_stack[page 1-3] Regular stack
 */
uint8_t kernel_stack[5 * PAGE_SIZE] __aligned(PAGE_SIZE) __data;
uint8_t user_stack[PAGE_SIZE] __aligned(PAGE_SIZE) __user_data;

char kernel_cmdline[PAGE_SIZE];

addr_range_t kern_addr_ranges[] = {
    { .name = ".text",   .base = VIRT_KERNEL_BASE, .flags = L1_PROT, .from = &__start_text,  .to = &__end_text   },
    { .name = ".data",   .base = VIRT_KERNEL_BASE, .flags = L1_PROT, .from = __start_data,   .to = __end_data   },
    { .name = ".bss",    .base = VIRT_KERNEL_BASE, .flags = L1_PROT, .from = __start_bss,    .to = __end_bss    },
    { .name = ".rodata", .base = VIRT_KERNEL_BASE, .flags = L1_PROT, .from = __start_rodata, .to = __end_rodata },
};

addr_range_t user_addr_ranges[] = {
    { .name = ".text.user", .base = VIRT_USER_BASE, .flags = L1_PROT_USER_RO, .from = __start_text_user, .to = __end_text_user },
    { .name = ".data.user", .base = VIRT_USER_BASE, .flags = L1_PROT_USER,    .from = __start_data_user, .to = __end_data_user },
    { .name = ".bss.user",  .base = VIRT_USER_BASE, .flags = L1_PROT_USER,    .from = __start_bss_user,  .to = __end_bss_user  },
};

addr_range_t init_addr_ranges[] = {
    { .name = ".text.init", .base = VIRT_IDENT_BASE, .flags = L1_PROT_RO, .from = __start_text_init, .to = __end_text_init },
    { .name = ".data.init", .base = VIRT_IDENT_BASE, .flags = L1_PROT,    .from = __start_data_init, .to = __end_data_init },
    { .name = ".bss.init",  .base = VIRT_IDENT_BASE, .flags = L1_PROT,    .from = __start_bss_init,  .to = __end_bss_init  },
};

static inline void display_addr_range(addr_range_t *r) {
    printk("%10s: VA: [0x%016lx - 0x%016lx] PA: [0x%08lx - 0x%08lx]\n",
           r->name, r->from, r->to, r->from - r->base, r->to - r->base);
}

void display_memory_map(void) {
    printk("Memory Map:\n");

    for (int i = 0; i < ARRAY_SIZE(kern_addr_ranges); i++)
        display_addr_range(&kern_addr_ranges[i]);

    for (int i = 0; i < ARRAY_SIZE(user_addr_ranges); i++)
        display_addr_range(&user_addr_ranges[i]);
}

static void init_console(void) {
    register_console_callback(serial_console_write);
}

static __always_inline void zero_bss(void) {
    memset(_ptr(__start_bss), 0x0, _ptr(__end_bss) - _ptr(__start_bss));
    memset(_ptr(__start_bss_user), 0x0, _ptr(__end_bss_user) - _ptr(__start_bss_user));
}

static __always_inline void zap_boot_mappings(void) {
#if defined (__x86_64__)
    memset(paddr_to_virt_kern(virt_to_paddr(l4_pt_entries)), 0, L4_PT_ENTRIES * sizeof(pgentry_t));
#endif
    memset(paddr_to_virt_kern(virt_to_paddr(l3_pt_entries)), 0, L3_PT_ENTRIES * sizeof(pgentry_t));
    memset(paddr_to_virt_kern(virt_to_paddr(l2_pt_entries)), 0, L2_PT_ENTRIES * sizeof(pgentry_t));
    memset(paddr_to_virt_kern(virt_to_paddr(l1_pt_entries)), 0, L1_PT_ENTRIES * sizeof(pgentry_t));
}

void __noreturn __text_init kernel_start(multiboot_info_t *mbi) {
    /* Zero-out BSS sections */
    zero_bss();

    /* Indentity mapping is still on, so fill in multiboot structures */
    init_multiboot(mbi);

    /* Initialize console early */
    init_console();

    /* Setup final pagetables */
    init_pagetables();

#if defined (__x86_64__)
    write_cr3(cr3.paddr);
#elif defined (__i386__)
    write_cr3(cr3.paddr);
#endif

    write_sp(_ul(GET_KERN_STACK()));

    /* TODO: Exception tables */

    init_traps();

    init_user_pagetables();

    zap_boot_mappings();

    /* TODO PerCPU support */

    /* TODO: SMP support */

    /* Jump from .text.init section to .text */
    asm volatile("push %0; ret" :: "r" (&kernel_main));

    UNREACHABLE();
}
