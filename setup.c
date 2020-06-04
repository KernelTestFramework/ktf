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

#include <smp/smp.h>

#include <drivers/serial.h>
#include <drivers/vga.h>

bool opt_debug;

io_port_t com_ports[2];

/*
 * KTF Stack layout:
 *
 * kernel_stack[page 5] Emergency stack
 * kernel_stack[page 4] Exception stack
 * kernel_stack[page 1-3] Regular stack
 */
uint8_t kernel_stack[7 * PAGE_SIZE] __aligned(PAGE_SIZE) __data;
uint8_t user_stack[PAGE_SIZE] __aligned(PAGE_SIZE) __user_data;

char kernel_cmdline[PAGE_SIZE];

#define VIDEO_START _ptr(VIRT_KERNEL_BASE + VGA_START_ADDR)
#define VIDEO_END  _ptr(VIRT_KERNEL_BASE + VGA_END_ADDR)
addr_range_t addr_ranges[] = {
    { .name = "Low memory", .base = VIRT_IDENT_BASE, .flags = L1_PROT_RO,      .from = _ptr(0x0),         .to = _ptr(MB(1))     },
    { .name = ".text.init", .base = VIRT_IDENT_BASE, .flags = L1_PROT_RO,      .from = __start_text_init, .to = __end_text_init },
    { .name = ".data.init", .base = VIRT_IDENT_BASE, .flags = L1_PROT,         .from = __start_data_init, .to = __end_data_init },
    { .name = ".bss.init",  .base = VIRT_IDENT_BASE, .flags = L1_PROT,         .from = __start_bss_init,  .to = __end_bss_init  },

    { .name = ".rmode",     .base = VIRT_IDENT_BASE, .flags = L1_PROT,         .from = __start_rmode,  .to = __end_rmode        },

    { .name = ".text.user", .base = VIRT_USER_BASE,  .flags = L1_PROT_USER_RO, .from = __start_text_user, .to = __end_text_user },
    { .name = ".data.user", .base = VIRT_USER_BASE,  .flags = L1_PROT_USER,    .from = __start_data_user, .to = __end_data_user },
    { .name = ".bss.user",  .base = VIRT_USER_BASE,  .flags = L1_PROT_USER,    .from = __start_bss_user,  .to = __end_bss_user  },

    { .name = ".text",   .base = VIRT_KERNEL_BASE,   .flags = L1_PROT_RO,      .from = __start_text,      .to = __end_text      },
    { .name = ".data",   .base = VIRT_KERNEL_BASE,   .flags = L1_PROT,         .from = __start_data,      .to = __end_data      },
    { .name = ".bss",    .base = VIRT_KERNEL_BASE,   .flags = L1_PROT,         .from = __start_bss,       .to = __end_bss       },
    { .name = ".rodata", .base = VIRT_KERNEL_BASE,   .flags = L1_PROT,         .from = __start_rodata,    .to = __end_rodata    },

    { .name = "VIDEO",   .base = VIRT_KERNEL_BASE,   .flags = L1_PROT,         .from = VIDEO_START,       .to = VIDEO_END       },

    { 0x0 } /* NULL array terminator */
};

void display_memory_map(void) {
    printk("Memory Map:\n");

    for_each_memory_range(r) {
        printk("%11s: VA: [0x%016lx - 0x%016lx] PA: [0x%08lx - 0x%08lx]\n",
               r->name, r->from, r->to, r->from - r->base, r->to - r->base);
    }

}

static void init_console(void) {
    get_com_ports();

    uart_init(com_ports[0], DEFAULT_BAUD_SPEED);

    register_console_callback(serial_console_write);
    register_console_callback(vga_console_write);

    printk("COM1: %x, COM2: %x\n", com_ports[0], com_ports[1]);
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

void __noreturn __text_init kernel_start(uint32_t multiboot_magic, multiboot_info_t *mbi) {
    /* Zero-out BSS sections */
    zero_bss();

    /* Initialize console early */
    init_console();

    if (multiboot_magic == MULTIBOOT_BOOTLOADER_MAGIC) {
        /* Indentity mapping is still on, so fill in multiboot structures */
        init_multiboot(mbi, &kernel_cmdline);
    }

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

    smp_init();

    /* Jump from .text.init section to .text */
    asm volatile("push %0; ret" :: "r" (&kernel_main));

    UNREACHABLE();
}
