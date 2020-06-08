#ifndef KTF_VMM_H
#define KTF_VMM_H

#include <page.h>

enum gfp_flags {
    GFP_KERNEL  = 0x00000001,
    GFP_USER    = 0x00000002,
    GFP_IDENT   = 0x00000004,
};

/* External definitions */

extern void *get_free_pages(unsigned int order, uint32_t flags);
extern void put_pages(void *page, unsigned int order);

/* Static definitions */

static inline void *get_free_page(uint32_t flags) {
    return get_free_pages(PAGE_ORDER_4K, flags);
}

static inline void *get_free_pages_top(unsigned int order, uint32_t flags) {
    return get_free_pages(order, flags) + (PAGE_SIZE << order);
}

static inline void *get_free_page_top(uint32_t flags) {
    return get_free_page(flags) + PAGE_SIZE;
}

static inline void put_page(void *page) {
    put_pages(page, PAGE_ORDER_4K);
}

#endif /* KTF_VMM_H */
