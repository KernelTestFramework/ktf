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
 *
 * In order to develop this slab allocator, inspiration has been taken from here
 * http://3zanders.co.uk/2018/02/24/the-slab-allocator/ but no code has been copied
 */
#include <console.h>
#include <errno.h>
#include <ktf.h>
#include <lib.h>
#include <mm/slab.h>
#include <mm/vmm.h>
#include <page.h>
#include <processor.h>
#include <sched.h>
#include <smp/smp.h>
#include <spinlock.h>
#include <string.h>

/* List of meta slab pointers of each order */
static list_head_t meta_slab_list[SLAB_ORDER_MAX];

static list_head_t meta_slab_page_list;
static spinlock_t slab_mm_lock = SPINLOCK_INIT;

static int initialize_slab(meta_slab_t *slab) {
    int ret = 0;
    unsigned int slab_count = 0, index = 0;
    slab_t *slab_entry = NULL;

    if (NULL == slab) {
        dprintk("failed, NULL slab\n");
        return -EINVAL;
    }

    if (NULL == slab->slab_base) {
        dprintk("failed, NULL base\n");
        return -EINVAL;
    }

    if (slab->slab_size > SLAB_SIZE_MAX) {
        dprintk("failed, large slab size\n");
        return -EINVAL;
    }

    if (slab->slab_size < SLAB_SIZE_MIN) {
        dprintk("failed, small slab size\n");
        return -EINVAL;
    }

    if ((slab->slab_size & (slab->slab_size - 1)) != 0) {
        dprintk("failed, slab size not power of 2\n");
        return -EINVAL;
    }

    slab_count = (slab->slab_len / slab->slab_size);
    list_init(&slab->slab_head);

    for (index = 0; index < slab_count; index++) {
        slab_entry = (slab_t *) (_ul(slab->slab_base) + (index * (slab->slab_size)));
        list_add_tail(&slab_entry->list, &slab->slab_head);
    }

    return ret;
}

static void *slab_alloc(meta_slab_t *slab) {
    slab_t *next_free = NULL;

    if (NULL == slab) {
        dprintk("failed, NULL slab param\n");
        return NULL;
    }

    if (list_is_empty(&slab->slab_head))
        return NULL;

    /* TODO: Below should be done in thread-safe manner */
    next_free = list_first_entry(&slab->slab_head, slab_t, list);
    list_unlink(&next_free->list);
    increment_slab_allocs(slab);
    return next_free;
}

static void slab_free(meta_slab_t *slab, void *ptr) {
    slab_t *new_slab = NULL;
    if (slab == NULL || ptr == NULL) {
        dprintk("NULL slab or ptr param\n");
        return;
    }

    new_slab = (slab_t *) ptr;
    /* TODO: eventually below should be done in thread-safe manner */
    list_add_tail(&new_slab->list, &slab->slab_head);
    decrement_slab_allocs(slab);
}

meta_slab_t *slab_meta_alloc() {
    meta_slab_t *meta_slab_page = NULL;
    meta_slab_t *meta_slab = NULL;
    void *free_page = NULL;
    size_t meta_slab_size = 0;
    int ret = 0;

    list_for_each_entry (meta_slab_page, &meta_slab_page_list, list) {
        meta_slab = slab_alloc(meta_slab_page);
        if (meta_slab != NULL) {
            dprintk("Allocating meta_slab from meta slab page %p\n", meta_slab_page);
            return meta_slab;
        }
    }

    /*
     * If we're here then we've ran out of meta slab pages
     * Allocate a 4K page
     */
    free_page = get_free_pages(PAGE_ORDER_4K, GFP_KERNEL);
    if (free_page == NULL) {
        dprintk("slab_meta_alloc failed, not enough free pages\n");
        return NULL;
    }
    memset(free_page, 0, PAGE_SIZE);

    /*
     * First entry in free page is special meta_slab
     * pointing to rest of meta_slabs
     */
    meta_slab_size = next_power_of_two(sizeof(meta_slab_t));
    meta_slab_page = (meta_slab_t *) free_page;
    meta_slab_page->slab_base = (void *) (_ul(meta_slab_page) + meta_slab_size);
    meta_slab_page->slab_len = PAGE_SIZE - meta_slab_size;
    meta_slab_page->slab_size = meta_slab_size;
    meta_slab_page->slab_allocs = 0;
    ret = initialize_slab(meta_slab_page);
    if (ret != ESUCCESS) {
        dprintk("initialize_slab in slab_meta_alloc failed\n");
        put_pages(free_page, PAGE_ORDER_4K);
        return NULL;
    }

    dprintk("Allocated a new meta page slab %p\n", meta_slab_page);
    /*
     * add meta_slab_page to global list of meta slab pages
     */
    list_add(&meta_slab_page->list, &meta_slab_page_list);

    /*
     * Now allocate a meta slab from meta slab page
     */
    meta_slab = slab_alloc(meta_slab_page);

    return meta_slab;
}
/*
 * Round up to nearest power of 2
 * If greater than max size or less than min size, return null
 */
static void *ktf_alloc(size_t size) {
    size_t size_power2 = 0, temp = 0, order_index = 0;
    meta_slab_t *slab = NULL, *meta_slab = NULL;
    void *alloc = NULL, *free_page = NULL;
    int ret = 0;

    if (size < SLAB_SIZE_MIN)
        size = SLAB_SIZE_MIN;

    size_power2 = next_power_of_two(size);
    if (size_power2 > SLAB_SIZE_MAX) {
        dprintk("failed, wrong slab size\n");
        return NULL;
    }

    temp = size_power2;
    order_index = log2(temp);
    /*
     * Decrement it by 4 because min size is 16 bytes and thus min
     * index will be 4, and we need to index into zero based array
     */
    order_index -= 4;

    dprintk("Alloc size %u, powerof 2 size %u, order %u\n", size, size_power2,
            order_index);
    spin_lock(&slab_mm_lock);
    /* Go through list of meta_slab_t and try to allocate a free slab */
    list_for_each_entry (slab, &meta_slab_list[order_index], list) {
        alloc = slab_alloc(slab);
        if (alloc != NULL) {
            dprintk("Allocating from %p\n", slab);
            goto out;
        }
    }

    /*
     * If we reached here that means it's time to allocate a new meta_slab_t entry
     * and a new page to hold base address
     */
    meta_slab = slab_meta_alloc();
    if (meta_slab == NULL) {
        dprintk("failed, not enough free pages\n");
        alloc = NULL;
        goto out;
    }

    dprintk("meta_slab allocated %p\n", meta_slab);

    /*
     * for now let's allocate 4K pages for all allocations,
     * we can change the logic in future to have 2M for larger slab sizes
     */

    free_page = get_free_pages(PAGE_ORDER_4K, GFP_KERNEL);
    if (free_page == NULL) {
        dprintk("ktf_alloc failed, not enough free pages\n");
        slab_free(META_SLAB_PAGE_ENTRY(meta_slab), meta_slab);
        alloc = NULL;
        goto out;
    }
    memset(free_page, 0, PAGE_SIZE);

    meta_slab->slab_base = free_page;
    meta_slab->slab_len = PAGE_SIZE;
    meta_slab->slab_size = size_power2;
    meta_slab->slab_allocs = 0;
    ret = initialize_slab(meta_slab);

    if (ret != ESUCCESS) {
        dprintk("initialize_slab failed\n");
        put_pages(free_page, PAGE_ORDER_4K);
        alloc = NULL;
        goto out;
    }

    list_add(&meta_slab->list, &meta_slab_list[order_index]);
    alloc = slab_alloc(meta_slab);

out:
    spin_unlock(&slab_mm_lock);
    return alloc;
}

void *kmalloc(size_t size) { return ktf_alloc(size); }

void *kzalloc(size_t size) {
    void *ptr = ktf_alloc(size);

    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

/*
 * Loop through all the orders and check where does this memory belong
 * Then link it back into free list. Not a O(1) of implementation but we're looking for
 * simple now
 */
static void ktf_free(void *ptr) {
    int alloc_order;
    meta_slab_t *slab = NULL;
    meta_slab_t *meta_slab_page = NULL;

    spin_lock(&slab_mm_lock);
    for (alloc_order = SLAB_ORDER_16; alloc_order < SLAB_ORDER_MAX; alloc_order++) {
        /* Go through list of meta_slab_t and try to allocate a free slab */
        list_for_each_entry (slab, &meta_slab_list[alloc_order], list) {
            if ((_ul(ptr) >= (_ul(slab->slab_base))) &&
                (_ul(ptr) < (_ul(slab->slab_base) + _ul(slab->slab_len)))) {
                slab_free(slab, ptr);
                if (slab_is_empty(slab)) {
                    dprintk("freeing slab %p of slab size %d and base address %p\n", slab,
                            slab->slab_size, slab->slab_base);
                    /*
                     * Order is important here. First unlink from order list
                     * Then only slab_free because list will be used to link back into
                     * meta slab free
                     */
                    list_unlink(&slab->list);
                    put_pages(slab->slab_base, PAGE_ORDER_4K);
                    meta_slab_page = META_SLAB_PAGE_ENTRY(slab);
                    slab_free(meta_slab_page, slab);
                    /*
                     * If page holding meta slabs is empty due to this operation, we
                     * should free up meta slab page entirely
                     */
                    if (slab_is_empty(meta_slab_page)) {
                        dprintk("freeing meta page slab %p of slab size %d and base "
                                "address %p\n",
                                meta_slab_page, meta_slab_page->slab_size,
                                meta_slab_page->slab_base);
                        list_unlink(&meta_slab_page->list);
                        memset(meta_slab_page, 0, PAGE_SIZE);
                        put_pages(meta_slab_page, PAGE_ORDER_4K);
                    }
                }
                spin_unlock(&slab_mm_lock);
                return;
            }
        }
    }

    panic("Attempted to free %p and couldn't find it\n", ptr);
    /* If we reached here, something terribly went wrong */
    UNREACHABLE();
}

void kfree(void *ptr) { ktf_free(ptr); }

int init_slab(void) {
    int ret = 0;
    int i = 0;

    printk("Initialize SLAB\n");
    spin_lock(&slab_mm_lock);
    memset(&meta_slab_list, 0, sizeof(meta_slab_list));
    memset(&meta_slab_page_list, 0, sizeof(meta_slab_page_list));

    list_init(&meta_slab_page_list);
    for (i = SLAB_ORDER_16; i < SLAB_ORDER_MAX; i++) {
        list_init(&meta_slab_list[i]);
    }
    spin_unlock(&slab_mm_lock);
    dprintk("After initializing slab module\n");
    return ret;
}
