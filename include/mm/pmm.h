/*
 * Copyright © 2020 Amazon.com, Inc. or its affiliates.
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
#ifndef KTF_PMM_H
#define KTF_PMM_H

#ifndef __ASSEMBLY__
#include <list.h>

#include <mm/regions.h>

struct frame {
    struct list_head list;
    mfn_t mfn;
    uint32_t refcount;
    uint32_t : 23, mapped : 1, order : 6, uncachable : 1, free : 1;
};
typedef struct frame frame_t;

#define for_each_order(order) for (int order = 0; order < MAX_PAGE_ORDER + 1; order++)

typedef bool (*free_frames_cond_t)(frame_t *free_frame);

/* External definitions */

extern void display_frames_count(void);
extern void init_pmm(void);

extern frame_t *get_free_frames_cond(free_frames_cond_t cb);
extern frame_t *get_free_frames(unsigned int order);
extern void put_frame(mfn_t mfn, unsigned int order);
extern void reclaim_frame(mfn_t mfn, unsigned int order);

extern void map_used_memory(void);

/* Static definitions */

static inline bool paddr_invalid(paddr_t pa) {
    return pa == PADDR_INVALID || !has_memory_range(pa);
}

static inline bool mfn_invalid(mfn_t mfn) { return paddr_invalid(mfn_to_paddr(mfn)); }

static inline frame_t *get_free_frame(void) { return get_free_frames(PAGE_ORDER_4K); }

#endif /* __ASSEMBLY__ */

#endif /* KTF_PMM_H */
