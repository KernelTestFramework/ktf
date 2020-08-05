/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
