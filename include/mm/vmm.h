/*
 * Copyright (c) 2020 Amazon.com, Inc. or its affiliates.
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
