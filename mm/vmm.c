#include <ktf.h>
#include <lib.h>
#include <page.h>

#include <mm/pmm.h>
#include <mm/vmm.h>

void *get_free_pages(unsigned int order, uint32_t flags) {
    mfn_t mfn = get_free_frames(order);
    void *va = NULL;

    if (mfn_invalid(mfn))
        return NULL;

    if (flags & GFP_IDENT)
        va = vmap(mfn_to_virt(mfn), mfn, order, L1_PROT);
    if (flags & GFP_USER)
        va = vmap(mfn_to_virt_user(mfn), mfn, order, L1_PROT_USER);
    if (flags & GFP_KERNEL)
        va = kmap(mfn, order, L1_PROT);

    return va;
}

void put_pages(void *page, unsigned int order) {
    /* FIXME: unmap all mappings */
    vunmap(page, order);
    put_frame(virt_to_mfn(page), order);
}
