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
#include <spinlock.h>

#include <mm/pmm.h>
#include <mm/regions.h>

size_t total_phys_memory;

static list_head_t free_frames[MAX_PAGE_ORDER + 1];
static list_head_t busy_frames[MAX_PAGE_ORDER + 1];

static frame_t early_frames[2 * PAGE_SIZE];
static unsigned int free_frame_idx;

static size_t frames_count[MAX_PAGE_ORDER + 1];

static spinlock_t lock = SPINLOCK_INIT;

void display_frames_count(void) {
    printk("Avail memory frames: (total size: %lu MB)\n", total_phys_memory / MB(1));

    for_each_order (order) {
        size_t count = frames_count[order];

        if (count)
            printk("  %lu KB: %lu\n", (PAGE_SIZE << order) / KB(1), count);
    }
}

static inline frame_t *new_frame(mfn_t mfn, unsigned int order) {
    frame_t *frame = &early_frames[free_frame_idx++];

    if (free_frame_idx > ARRAY_SIZE(early_frames))
        panic("Not enough initial frames for PMM allocation!\n");

    frame->order = order;
    frame->mfn = mfn;
    frame->flags.free = true;

    frames_count[order]++;
    return frame;
}

static inline void add_early_frame(mfn_t mfn, unsigned int order) {
    frame_t *frame = new_frame(mfn, order);

    list_add(&frame->list, &free_frames[order]);
}

static inline void add_frame(mfn_t mfn, unsigned int order) {
    frame_t *frame = new_frame(mfn, order);

    list_add_tail(&frame->list, &free_frames[order]);
}

static size_t process_memory_range(unsigned index) {
    paddr_t start, end, cur;
    addr_range_t range;
    size_t size;

    if (get_avail_memory_range(index, &range) < 0)
        return 0;

    /* Find unused beginning of the region */
    for (start = _paddr(range.start); !in_free_region(start); start += PAGE_SIZE)
        ;

    cur = start;
    end = _paddr(range.end);
    size = end - start;

    /*
     * It's important to add the initial frames to the front of the list,
     * because initial virtual memory mapping is small.
     */

    /* Add initial 4K frames and align to 2M. */
    while (cur % PAGE_SIZE_2M && cur + PAGE_SIZE <= end) {
        if (index <= 1)
            add_early_frame(paddr_to_mfn(cur), PAGE_ORDER_4K);
        else
            add_frame(paddr_to_mfn(cur), PAGE_ORDER_4K);
        cur += (PAGE_SIZE << PAGE_ORDER_4K);
    }

    /* Add initial 2M frames and align to 1G. */
    while (cur % PAGE_SIZE_1G && cur + PAGE_SIZE_2M <= end) {
        add_frame(paddr_to_mfn(cur), PAGE_ORDER_2M);
        cur += (PAGE_SIZE << PAGE_ORDER_2M);
    }

    /* Add all remaining 1G frames. */
    while (cur + PAGE_SIZE_1G <= end) {
        add_frame(paddr_to_mfn(cur), PAGE_ORDER_1G);
        cur += (PAGE_SIZE << PAGE_ORDER_1G);
    }

    /* Add all remaining 2M frames. */
    while (cur + PAGE_SIZE_2M <= end) {
        add_frame(paddr_to_mfn(cur), PAGE_ORDER_2M);
        cur += (PAGE_SIZE << PAGE_ORDER_2M);
    }

    /* Add all remaining 4K frames. */
    while (cur < end) {
        add_frame(paddr_to_mfn(cur), PAGE_ORDER_4K);
        cur += (PAGE_SIZE << PAGE_ORDER_4K);
    }

    if (cur != end) {
        panic(
            "PMM range processing failed: start=0x%016lx end=0x%016lx current=0x%016lx\n",
            start, end, cur);
    }

    return size;
}

static inline void display_frames(void) {
    printk("List of frames:\n");
    for_each_order (order) {
        if (!list_is_empty(&free_frames[order])) {
            frame_t *frame;

            printk("Order: %u\n", order);
            list_for_each_entry (frame, &free_frames[order], list)
                display_frame(frame);
        }
    }
}

void reclaim_frame(mfn_t mfn, unsigned int order) { add_frame(mfn, order); }

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
    if (opt_debug)
        display_frames();
}

static inline frame_t *reserve_frame(frame_t *frame) {
    if (!frame)
        return NULL;

    if (frame->refcount++ == 0) {
        list_unlink(&frame->list);
        list_add(&frame->list, &busy_frames[frame->order]);
    }

    return frame;
}

static inline frame_t *return_frame(frame_t *frame) {
    if (!is_frame_used(frame))
        panic("PMM: trying to return unused frame: %p\n", frame);

    if (--frame->refcount == 0) {
        list_unlink(&frame->list);
        list_add(&frame->list, &free_frames[frame->order]);
    }

    return frame;
}

/* Reserves and returns the first free frame fulfilling
 * the condition specified by the callback
 */
frame_t *get_free_frames_cond(free_frames_cond_t cb) {
    frame_t *frame;

    spin_lock(&lock);
    for_each_order (order) {
        if (list_is_empty(&free_frames[order]))
            continue;

        list_for_each_entry (frame, &free_frames[order], list) {
            if (cb(frame)) {
                spin_unlock(&lock);
                return reserve_frame(frame);
            }
        }
    }
    spin_unlock(&lock);

    return NULL;
}

frame_t *get_free_frames(unsigned int order) {
    frame_t *frame;

    if (order > MAX_PAGE_ORDER)
        return NULL;

    spin_lock(&lock);
    if (list_is_empty(&free_frames[order])) {
        /* FIXME: Add page split */
        spin_unlock(&lock);
        return NULL;
    }

    frame = reserve_frame(list_first_entry(&free_frames[order], frame_t, list));
    spin_unlock(&lock);

    return frame;
}

void put_free_frames(mfn_t mfn, unsigned int order) {
    frame_t *frame;

    BUG_ON(mfn_invalid(mfn));

    if (order > MAX_PAGE_ORDER)
        return;

    spin_lock(&lock);
    list_for_each_entry (frame, &busy_frames[order], list) {
        if (frame->mfn == mfn) {
            /* FIXME: Maintain order wrt mfn value */
            /* FIXME: Add frame merge */
            return_frame(frame);
            spin_unlock(&lock);
            return;
        }
    }
    spin_unlock(&lock);

    panic("PMM: unable to find frame: %x in busy frames list\n");
}

void map_used_memory(void) {
    frame_t *frame;

    for_each_order (order) {
        list_for_each_entry (frame, &busy_frames[order], list) {
            if (!frame->flags.mapped) {
                kmap(frame->mfn, order, L4_PROT, L3_PROT, L2_PROT, L1_PROT);
                frame->flags.mapped = true;
            }
        }
    }
}
