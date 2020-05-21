#include <ktf.h>
#include <lib.h>
#include <page.h>
#include <setup.h>
#include <console.h>
#include <multiboot.h>

#include <smp/mptables.h>

static inline uint8_t get_mp_checksum(void *ptr, size_t len) {
    uint8_t checksum = 0;

    for (int i = 0; i < len; i++)
        checksum += *((uint8_t *) ptr + i);

    return checksum;
}

static inline bool validate_mpf(mpf_t *ptr) {
    if (ptr->signature != MPF_SIGNATURE)
        return false;

    /* MP Floating Pointer Structure is always 16 bytes long */
    if (ptr->length != 0x01)
        return false;

    if (ptr->spec_rev < 0x01 || ptr->spec_rev > 0x04)
        return false;

    if (get_mp_checksum(ptr, ptr->length * sizeof(*ptr)) != 0x00)
        return false;

    return true;
}

static inline mpf_t *find_mpf(void *from, void *to) {
    /* MP Floating Pointer Structure is 16 bytes aligned */
    mpf_t *addr = _ptr(_ul(from) & ~_ul(0x10));

    while (_ptr(addr) < to) {
        if (validate_mpf(addr))
            return addr;
        addr++;
    }

    return NULL;
}

static mpf_t *get_mpf_addr(void) {
    uint32_t ebda_addr;
    mpf_t *ptr;

    ebda_addr = (* (uint16_t *) paddr_to_virt_kern(EBDA_ADDR_ENTRY)) << 4;
    ptr = find_mpf(paddr_to_virt_kern(ebda_addr), paddr_to_virt_kern(ebda_addr + KB(1)));
    if (ptr)
        return ptr;

    ptr = find_mpf(paddr_to_virt_kern(mbi_lower_memory()), paddr_to_virt_kern(mbi_lower_memory() + KB(1)));
    if (ptr)
        return ptr;

    return find_mpf(paddr_to_virt_kern(BIOS_ROM_ADDR_START), paddr_to_virt_kern(BIOS_ROM_ADDR_START + KB(64)));
}

static void dump_mpf(mpf_t *ptr) {
    printk("\nMP Floating Pointer Structure at %p:\n", ptr);
    printk("  Signature: %.4s\n", &ptr->signature);
    printk("  MP Configuration Table phys address: 0x%08x\n", ptr->mpc_base);
    printk("  Length: 0x%02x\n", ptr->length);
    printk("  Spec revision: 0x%02x\n", ptr->spec_rev);
    printk("  Checksum: 0x%02x\n", ptr->checksum);
    printk("  MP System Config Type: 0x%02x %s\n", ptr->mpc_type,
           ptr->mpc_type? "(Default Config Code)" : "(MP Config Table)");
    printk("  Mode implemented: %s\n", ptr->imcrp ? "PIC Mode" : "Virtual Wire Mode");
}

void mptables_init(void) {
    mpf_t *mpf_ptr = get_mpf_addr();

    if (!mpf_ptr) {
        printk("No MP Floating Structure Pointer found!\n");
        return;
    }

    dump_mpf(mpf_ptr);
}
