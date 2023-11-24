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
#include <mm/vmm.h>
#include <pagetable.h>
#include <setup.h>
#include <spinlock.h>

size_t total_phys_memory;

static list_head_t frames;
static unsigned long total_free_frames = 0;
#define MIN_FREE_FRAMES_THRESHOLD 2
#define MAX_FREE_FRAMES_THRESHOLD (2 * ARRAY_SIZE(memberof(frames_array_t, frames)))
static frames_array_t early_frames;

static list_head_t free_frames[MAX_PAGE_ORDER + 1];
static list_head_t busy_frames[MAX_PAGE_ORDER + 1];

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

static inline void init_frame(frame_t *frame) {
    ASSERT(frame);

    memset(frame, 0, sizeof(*frame));
    frame->order = PAGE_ORDER_INVALID;
    frame->flags.free = true;
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

static inline bool return_frame(frame_t *frame) {
    ASSERT(is_frame_used(frame));

    if (--frame->refcount == 0) {
        list_unlink(&frame->list);
        list_add(&frame->list, &free_frames[frame->order]);
        return true;
    }

    return false;
}

static inline void init_frames_array(frames_array_t *array) {
    memset(array, 0, sizeof(*array));
    array->meta.free_count = ARRAY_SIZE(array->frames);
    for (unsigned i = 0; i < ARRAY_SIZE(array->frames); i++)
        init_frame(&array->frames[i]);
    list_add(&array->list, &frames);
}

static frames_array_t *new_frames_array(void) {
    frames_array_t *array;
    frame_t *frame;

    frame = reserve_frame(get_first_frame(free_frames, PAGE_ORDER_4K));
    if (!frame)
        goto error;

    if (!boot_flags.virt)
        array = (frames_array_t *) mfn_to_virt_kern(frame->mfn);
    else {
        array = vmap_kern_4k(mfn_to_virt_map(frame->mfn), frame->mfn, L1_PROT);
        if (!array)
            goto error;
    }

    dprintk("%s: allocated new frames array: %p\n", __func__, array);

    init_frames_array(array);

    total_free_frames += array->meta.free_count;
    return array;
error:
    panic("PMM: Unable to allocate new page for frame array");
    UNREACHABLE();
}

static void del_frames_array(frames_array_t *array) {
    ASSERT(array);

    list_unlink(&array->list);
    total_free_frames -= array->meta.free_count;
    put_free_frame(virt_to_mfn(array));

    dprintk("%s: freed frames array: %p\n", __func__, array);
}

static bool is_frames_array_free(const frames_array_t *array) {
    ASSERT(array);

    if (array->meta.free_count < ARRAY_SIZE(array->frames))
        return false;
    else if (array->meta.free_count > ARRAY_SIZE(array->frames))
        panic("PMM: incorrect number of free slots: %d in array: %p",
              array->meta.free_count, array);

    for (unsigned i = 0; i < ARRAY_SIZE(array->frames); i++)
        ASSERT(is_frame_free(&array->frames[i]));

    return true;
}

static inline frames_array_t *get_frames_array(void) {
    frames_array_t *array;

    list_for_each_entry (array, &frames, list) {
        if (array->meta.free_count > 0)
            return array;
    }

    return new_frames_array();
}

static inline bool put_frames_array(frames_array_t *array) {
    if (is_frames_array_free(array)) {
        del_frames_array(array);
        return true;
    }

    return false;
}

static inline frames_array_t *find_frames_array(const frame_t *frame) {
    frames_array_t *array;
    mfn_t frame_mfn = virt_to_mfn(frame);

    list_for_each_entry (array, &frames, list) {
        if (virt_to_mfn(array) == frame_mfn)
            return array;
    }

    return NULL;
}

static inline frame_t *take_frame(frame_t *frame, frames_array_t *array) {
    ASSERT(frame);

    if (!array)
        array = find_frames_array(frame);
    BUG_ON(!array);

    frame->flags.free = false;
    array->meta.free_count--;

    if (--total_free_frames <= MIN_FREE_FRAMES_THRESHOLD)
        new_frames_array();

    return frame;
}

static inline frame_t *put_frames_array_entry(frame_t *frame, frames_array_t *array) {
    ASSERT(!is_frame_free(frame));

    if (!array)
        array = find_frames_array(frame);
    BUG_ON(!array);

    array->meta.free_count++;

    if (++total_free_frames >= MAX_FREE_FRAMES_THRESHOLD)
        put_frames_array(array);

    init_frame(frame);
    return frame;
}

static inline frame_t *get_frames_array_entry(void) {
    frames_array_t *array = get_frames_array();

    if (!array)
        panic("PMM: Unable to get a free array of frames' metadata");

    for (unsigned i = 0; i < ARRAY_SIZE(array->frames); i++) {
        frame_t *frame = &array->frames[i];

        if (is_frame_free(frame))
            return take_frame(frame, array);
    }

    return NULL;
}

static inline void destroy_frame(frame_t *frame) {
    ASSERT(!is_frame_used(frame));

    if (frame) {
        list_unlink(&frame->list);
        frames_count[frame->order]--;

        put_frames_array_entry(frame, NULL);
    }
}

static inline frame_t *new_frame(mfn_t mfn, unsigned int order) {
    frame_t *frame = get_frames_array_entry();

    frame->order = order;
    frame->mfn = mfn;

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

static inline unsigned int find_max_avail_order(size_t size) {
    for (unsigned int order = MAX_PAGE_ORDER; order > PAGE_ORDER_4K; order--) {
        if (ORDER_TO_SIZE(order) <= size)
            return order;
    }

    return PAGE_ORDER_4K;
}

static unsigned find_first_avail_region(void) {
    addr_range_t range;

    for (unsigned int i = 0; i < regions_num; i++) {
        if (get_avail_memory_range(i, &range) < 0)
            continue;

        if (_paddr(range.start) < _paddr(MB(1)))
            continue;

        if (_paddr(range.end) - _paddr(range.start) < MB(EARLY_VIRT_MEM))
            continue;

        return i;
    }

    panic("PMM: Cannot obtain first available physical memory address range");
    UNREACHABLE();
}

static size_t process_memory_range(unsigned index, unsigned first_avail_region) {
    paddr_t start, end, cur;
    unsigned int max_order;
    addr_range_t range;
    size_t size;

    if (get_avail_memory_range(index, &range) < 0)
        return 0;

    start = get_region_free_start(range.start);
    cur = start;
    end = _paddr(range.end);
    size = end - start;

    /*
     * It's important to add the initial frames to the front of the list,
     * because initial virtual memory mapping is small.
     */

    /* Add initial 4K frames and align to 2M. */
    while ((cur < MB(EARLY_VIRT_MEM) || cur % PAGE_SIZE_2M) && cur + PAGE_SIZE <= end) {
        if (index <= first_avail_region)
            add_early_frame(paddr_to_mfn(cur), PAGE_ORDER_4K);
        else
            add_frame(paddr_to_mfn(cur), PAGE_ORDER_4K);
        cur += ORDER_TO_SIZE(PAGE_ORDER_4K);
    }

    max_order = find_max_avail_order(end - cur);

    /* Add all available max_order frames. */
    while (cur + ORDER_TO_SIZE(max_order) <= end) {
        add_frame(paddr_to_mfn(cur), max_order);
        cur += ORDER_TO_SIZE(max_order);
    }

    /* Add all remaining 2M frames. */
    while (cur + PAGE_SIZE_2M <= end) {
        add_frame(paddr_to_mfn(cur), PAGE_ORDER_2M);
        cur += ORDER_TO_SIZE(PAGE_ORDER_2M);
    }

    /* Add all remaining 4K frames. */
    while (cur < end) {
        add_frame(paddr_to_mfn(cur), PAGE_ORDER_4K);
        cur += ORDER_TO_SIZE(PAGE_ORDER_4K);
    }

    if (cur != end) {
        warning(
            "PMM range processing failed: start=0x%016lx end=0x%016lx current=0x%016lx",
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

void reclaim_frame(mfn_t mfn, unsigned int order) {
    add_frame(mfn, order);
}

static inline void check_early_frames(unsigned first_avail_region) {
    unsigned early_frames_cnt;
    addr_range_t range;

    if (get_avail_memory_range(first_avail_region, &range) < 0)
        panic("PMM: Cannot obtain first available physical memory address range");

    early_frames_cnt =
        (MB(EARLY_VIRT_MEM) - get_region_free_start(range.start)) / PAGE_SIZE;
    if (frames_count[PAGE_ORDER_4K] < early_frames_cnt) {
        panic("Not enough early frames: %u missing",
              early_frames_cnt - frames_count[PAGE_ORDER_4K]);
    }
}

void init_pmm(void) {
    unsigned first_region_index;

    printk("Initialize Physical Memory Manager\n");

    BUILD_BUG_ON(sizeof(frames_array_t) > PAGE_SIZE);
    list_init(&frames);
    init_frames_array(&early_frames);

    BUILD_BUG_ON(ARRAY_SIZE(free_frames) != ARRAY_SIZE(busy_frames));
    for_each_order (order) {
        list_init(&free_frames[order]);
        list_init(&busy_frames[order]);
    }

    first_region_index = find_first_avail_region();

    /* Skip low memory range */
    for (unsigned int i = first_region_index; i < regions_num; i++)
        total_phys_memory += process_memory_range(i, first_region_index);

    display_frames_count();

    check_early_frames(first_region_index);

    if (opt_debug)
        display_frames();
}

static frame_t *_find_mfn_frame(list_head_t *list, mfn_t mfn, unsigned int order) {
    frame_t *frame;

    if (!has_frames(list, order))
        return NULL;

    list_for_each_entry (frame, &list[order], list) {
        if (frame->mfn == mfn)
            return frame;
    }

    return NULL;
}

frame_t *find_free_mfn_frame(mfn_t mfn, unsigned int order) {
    frame_t *frame;

    spin_lock(&lock);
    frame = _find_mfn_frame(free_frames, mfn, order);
    spin_unlock(&lock);

    return frame;
}

frame_t *find_busy_mfn_frame(mfn_t mfn, unsigned int order) {
    frame_t *frame;

    spin_lock(&lock);
    frame = _find_mfn_frame(busy_frames, mfn, order);
    spin_unlock(&lock);

    return frame;
}

frame_t *find_mfn_frame(mfn_t mfn, unsigned int order) {
    frame_t *frame;

    spin_lock(&lock);
    frame = _find_mfn_frame(busy_frames, mfn, order);
    if (!frame)
        frame = _find_mfn_frame(free_frames, mfn, order);
    spin_unlock(&lock);

    return frame;
}

static frame_t *_find_paddr_frame(list_head_t *list, paddr_t paddr) {
    frame_t *frame;

    for_each_order (order) {
        list_for_each_entry (frame, &list[order], list) {
            if (frame_has_paddr(frame, paddr))
                return frame;
        }
    }

    return NULL;
}

frame_t *find_free_paddr_frame(paddr_t paddr) {
    frame_t *frame;

    spin_lock(&lock);
    frame = _find_paddr_frame(free_frames, paddr);
    spin_unlock(&lock);

    return frame;
}

frame_t *find_busy_paddr_frame(paddr_t paddr) {
    frame_t *frame;

    spin_lock(&lock);
    frame = _find_paddr_frame(busy_frames, paddr);
    spin_unlock(&lock);

    return frame;
}

frame_t *find_paddr_frame(paddr_t paddr) {
    frame_t *frame;

    spin_lock(&lock);
    frame = _find_paddr_frame(busy_frames, paddr);
    if (!frame)
        frame = _find_paddr_frame(free_frames, paddr);
    spin_unlock(&lock);

    return frame;
}

static frame_t *find_larger_frame(list_head_t *list, unsigned int order) {
    while (++order <= MAX_PAGE_ORDER) {
        frame_t *frame = get_first_frame(list, order);

        if (frame)
            return frame;
    }

    return NULL;
}

static inline void relink_frame_to_order(frame_t *frame, unsigned int new_order) {
    ASSERT(new_order <= MAX_PAGE_ORDER);

    list_unlink(&frame->list);
    frames_count[frame->order]--;

    frame->order = new_order;

    list_add_tail(&frame->list, &free_frames[frame->order]);
    frames_count[frame->order]++;
}

static void split_frame(frame_t *frame) {
    BUG_ON(!frame);

    if (opt_debug) {
        printk("PMM: Splitting frame:\n");
        display_frame(frame);
    }

    /* First sibling frame */
    relink_frame_to_order(frame, frame->order - 1);

    /* Create new frame entry for the second sibling frame */
    add_frame(NEXT_MFN(frame->mfn, frame->order), frame->order);
}

static void merge_frames(frame_t *first) {
    frame_t *second;

    BUG_ON(!first);

    if (FIRST_FRAME_SIBLING(first->mfn, first->order + 1)) {
        mfn_t next_mfn = NEXT_MFN(first->mfn, first->order);
        second = _find_mfn_frame(free_frames, next_mfn, first->order);
    }
    else {
        /* Second frame sibling */
        mfn_t prev_mfn = PREV_MFN(first->mfn, first->order);
        second = first;
        first = _find_mfn_frame(free_frames, prev_mfn, first->order);
    }

    if (!first || !second)
        return;

    if (opt_debug) {
        printk("PMM: Merging frames:\n");
        display_frame(first);
        display_frame(second);
    }

    /* Make the first sibling a higher order frame */
    relink_frame_to_order(first, first->order + 1);

    /* Destroy the second sibling frame */
    destroy_frame(second);

    /* Try to merge higher order frames */
    merge_frames(first);
}

/* Reserves and returns the first free frame fulfilling
 * the condition specified by the callback.
 * This function does not split larger frames.
 */
frame_t *get_free_frames_cond(free_frames_cond_t cb) {
    spin_lock(&lock);
    for_each_order (order) {
        frame_t *frame;

        if (list_is_empty(&free_frames[order])) {
            continue;
        }

        list_for_each_entry (frame, &free_frames[order], list) {
            if (cb(frame)) {
                reserve_frame(frame);
                spin_unlock(&lock);
                return frame;
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
    while (list_is_empty(&free_frames[order])) {
        frame = find_larger_frame(free_frames, order);
        if (!frame) {
            spin_unlock(&lock);
            return NULL;
        }
        split_frame(frame);
    }

    frame = reserve_frame(get_first_frame(free_frames, order));
    spin_unlock(&lock);

    return frame;
}

void put_free_frames(mfn_t mfn, unsigned int order) {
    frame_t *frame;

    ASSERT(order <= MAX_PAGE_ORDER);

    spin_lock(&lock);
    frame = _find_mfn_frame(busy_frames, mfn, order);
    if (!frame) {
        warning("PMM: unable to find frame: %lx, order: %u among busy frames", mfn,
                order);
        goto unlock;
    }

    if (return_frame(frame))
        merge_frames(frame);

unlock:
    spin_unlock(&lock);
}

void map_frames_array(void) {
    frames_array_t *array;

    list_for_each_entry (array, &frames, list) {
        mfn_t mfn = virt_to_mfn(array);
        void *va = IS_ADDR_SPACE_VA(array, VIRT_KERNEL_BASE) ? mfn_to_virt_kern(mfn)
                                                             : mfn_to_virt_map(mfn);

        BUG_ON(!vmap_kern_4k(va, mfn, L1_PROT));
    }
}
