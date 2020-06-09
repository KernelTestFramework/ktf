#include <ktf.h>
#include <lib.h>
#include <page.h>
#include <console.h>
#include <processor.h>

#include <apic.h>

apic_mode_t apic_mode = APIC_MODE_UNKNOWN;

static const char *apic_mode_names[] = {
    [APIC_MODE_UNKNOWN]  = "Unknown",
    [APIC_MODE_NONE]     = "None",
    [APIC_MODE_DISABLED] = "Disabled",
    [APIC_MODE_XAPIC]    = "XAPIC",
    [APIC_MODE_X2APIC]   = "X2APIC",
};

int init_apic(enum apic_mode mode) {
    uint64_t apic_base = rdmsr(MSR_APIC_BASE);

    ASSERT(mode > APIC_MODE_NONE && mode <= APIC_MODE_X2APIC);

    if ((apic_base & PAGE_MASK) != DEFAULT_APIC_BASE) {
        printk("%s: unsupported APIC base: %lx\n", apic_base);
        return -1;
    }

    /* Boot up initialization */
    if (apic_mode == APIC_MODE_UNKNOWN) {
        switch (apic_base & (APIC_BASE_EXTD | APIC_BASE_ENABLE)) {
        case 0:
            apic_mode = APIC_MODE_DISABLED;
            break;

        case APIC_BASE_ENABLE:
            apic_mode = APIC_MODE_XAPIC;
            break;

        case APIC_BASE_EXTD | APIC_BASE_ENABLE:
            apic_mode = APIC_MODE_X2APIC;
            break;

        default:
            BUG();
        }
    }

    printk("Initializing APIC mode: %s -> %s\n",
           apic_mode_names[apic_mode], apic_mode_names[mode]);

    /* Disable APIC */
    apic_base &= ~(APIC_BASE_EXTD | APIC_BASE_ENABLE);
    wrmsr(MSR_APIC_BASE, apic_base);

    if (mode >= APIC_MODE_XAPIC)
        wrmsr(MSR_APIC_BASE, apic_base | APIC_BASE_ENABLE);

    if (mode == APIC_MODE_X2APIC)
        wrmsr(MSR_APIC_BASE, apic_base | APIC_BASE_EXTD | APIC_BASE_ENABLE);

    apic_mode = mode;

    /* XAPIC requires MMIO accesses, thus the APIC_BASE page needs to be mapped.
     * X2APIC uses MSRs for accesses, so no mapping needed.
     */
    if (apic_mode == APIC_MODE_XAPIC)
        vmap(_ptr(DEFAULT_APIC_BASE), paddr_to_mfn(DEFAULT_APIC_BASE), PAGE_ORDER_4K, L1_PROT);

    apic_write(APIC_SPIV, APIC_SPIV_APIC_ENABLED | 0xff);
    return 0;
}
