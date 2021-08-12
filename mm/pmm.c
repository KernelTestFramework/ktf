/*
 * Copyright © 2021 Amazon.com, Inc. or its affiliates.
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
#include <list.h>

#include <mm/pmm.h>
#include <mm/regions.h>

size_t total_phys_memory;

static list_head_t free_frames[MAX_PAGE_ORDER + 1];
static list_head_t busy_frames[MAX_PAGE_ORDER + 1];

static frame_t early_frames[2 * PAGE_SIZE];
static unsigned int free_frame_idx;

static size_t frames_count[MAX_PAGE_ORDER + 1];

void display_frames_count(void) {
    printk("Avail memory frames: (total size: %lu MB)\n", total_phys_memory / MB(1));

    for_each_order (order) {
        size_t count = frames_count[order];

        if (count)
            printk("  %lu KB: %lu\n", (PAGE_SIZE << order) / KB(1), count);
    }
}

static inline void display_frame(const frame_t *frame) {
    printk("Frame: mfn: %lx, order: %u, refcnt: %u, uc: %u, free: %u\n", frame->mfn,
           frame->order, frame->refcount, frame->uncachable, frame->free);
}

static void add_frame(paddr_t *pa, unsigned int order, bool initial) {
    frame_t *free_frame = &early_frames[free_frame_idx++];

    if (free_frame_idx > ARRAY_SIZE(early_frames))
        panic("Not enough initial frames for PMM allocation!\n");

    free_frame->order = order;
    free_frame->mfn = paddr_to_mfn(*pa);
    free_frame->free = true;

    *pa += (PAGE_SIZE << order);

    if (initial)
        list_add(&free_frame->list, &free_frames[order]);
    else
        list_add_tail(&free_frame->list, &free_frames[order]);
    frames_count[order]++;
}

void reclaim_frame(mfn_t mfn, unsigned int order) {
    paddr_t pa = mfn_to_paddr(mfn);

    add_frame(&pa, order, false);
}

static size_t process_memory_range(unsigned index) {
    paddr_t start, end, cur;
    addr_range_t range;
    size_t size;

    if (get_avail_memory_range(index, &range) < 0)
        return 0;

    cur = start = (index == 1 ? virt_to_paddr(__end_rodata) : _paddr(range.start));
    end = _paddr(range.end);
    size = end - start;

    /*
     * It's important to add the initial frames to the front of the list,
     * because initial virtual memory mapping is small.
     */

    /* Add initial 4K frames and align to 2M. */
    while (cur % PAGE_SIZE_2M && cur + PAGE_SIZE <= end)
        add_frame(&cur, PAGE_ORDER_4K, true);

    /* Add initial 2M frames and align to 1G. */
    while (cur % PAGE_SIZE_1G && cur + PAGE_SIZE_2M <= end)
        add_frame(&cur, PAGE_ORDER_2M, false);

    /* Add all remaining 1G frames. */
    while (cur + PAGE_SIZE_1G <= end)
        add_frame(&cur, PAGE_ORDER_1G, false);

    /* Add all remaining 2M frames. */
    while (cur + PAGE_SIZE_2M <= end)
        add_frame(&cur, PAGE_ORDER_2M, false);

    /* Add all remaining 4K frames. */
    while (cur < end)
        add_frame(&cur, PAGE_ORDER_4K, false);

    if (cur != end) {
        panic(
            "PMM range processing failed: start=0x%016lx end=0x%016lx current=0x%016lx\n",
            start, end, cur);
    }

    return size;
}

void init_pmm(void) {
    printk("Initialize Physical Memory Manager\n");

    BUG_ON(ARRAY_SIZE(free_frames) != ARRAY_SIZE(busy_frames));
    for_each_order (order) {
        list_init(&free_frames[order]);
        list_init(&busy_frames[order]);
    }

    /* Skip low memory range */
    for (unsigned int i = 1; i < regions_num; i++)
        total_phys_memory += process_memory_range(i);

    display_frames_count();

    if (opt_debug) {
        frame_t *frame;

        printk("List of frames:\n");
        for_each_order (order) {
            if (list_is_empty(&free_frames[order]))
                continue;

            printk("Order: %u\n", order);
            list_for_each_entry (frame, &free_frames[order], list)
                display_frame(frame);
        }
    }
}

static frame_t *reserve_frame(frame_t *frame, unsigned int order) {
    BUG_ON(!frame);
    BUG_ON(!frame->free);
    frame->free = false;

    BUG_ON(frame->refcount > 0);
    frame->refcount++;

    list_unlink(&frame->list);
    list_add(&frame->list, &busy_frames[order]);

    return frame;
}

/* Reserves and returns the first free frame
 * fulfilling the condition specified by
 * the callback
 */
frame_t *get_free_frames_cond(free_frames_cond_t cb) {
    frame_t *frame;

    for_each_order (order) {
        if (list_is_empty(&free_frames[order]))
            continue;

        list_for_each_entry (frame, &free_frames[order], list) {
            if (cb(frame)) {
                return reserve_frame(frame, order);
            }
        }
    }
    return NULL;
}

frame_t *get_free_frames(unsigned int order) {
    frame_t *frame;

    if (order > MAX_PAGE_ORDER)
        return NULL;

    if (list_is_empty(&free_frames[order])) {
        /* FIXME: Add page split */
        return NULL;
    }

    frame = list_first_entry(&free_frames[order], frame_t, list);

    frame = reserve_frame(frame, order);

    return frame;
}

void put_frame(mfn_t mfn, unsigned int order) {
    frame_t *frame;
    frame_t *found = NULL;

    if (mfn == MFN_INVALID)
        return;

    list_for_each_entry (frame, &busy_frames[order], list) {
        if (frame->mfn == mfn) {
            found = frame;
            break;
        }
    }

    BUG_ON(!found);

    BUG_ON(frame->refcount == 0);
    frame->refcount--;

    if (found->refcount > 0)
        return;

    BUG_ON(frame->free);
    frame->free = true;

    list_unlink(&frame->list);
    /* FIXME: Maintain order wrt mfn value */
    list_add(&frame->list, &free_frames[order]);
    /* FIXME: Add frame merge */
}

void map_used_memory(void) {
    frame_t *frame;

    for_each_order (order) {
        list_for_each_entry (frame, &busy_frames[order], list) {
            if (!frame->mapped) {
                kmap(frame->mfn, order, L4_PROT, L3_PROT, L2_PROT, L1_PROT);
                frame->mapped = true;
            }
        }
    }
}
