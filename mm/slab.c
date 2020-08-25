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
#include <mm/vmm.h>
#include <page.h>
#include <processor.h>
#include <sched.h>
#include <slab.h>
#include <smp/smp.h>
#include <string.h>

/* List of meta slab pointers of each order */
static list_head_t meta_slab_list[SLAB_ORDER_MAX];

meta_slab_t global_meta_slab;

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
}

/*
 * Round up to nearest power of 2
 * If greater than max size or less than min size, return null
 */
void *ktf_alloc(unsigned int size) {
    unsigned int size_power2 = 0, temp = 0, order_index = 0;
    meta_slab_t *slab = NULL, *meta_slab = NULL;
    void *alloc = NULL, *free_page = NULL;
    int ret = 0;

    if (size == 0)
        return NULL;

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

    printk("Alloc size %u, powerof 2 size %u, order %u\n", size, size_power2,
           order_index);
    /* Go through list of meta_slab_t and try to allocate a free slab */
    list_for_each_entry (slab, &meta_slab_list[order_index], list) {
        alloc = slab_alloc(slab);
        if (alloc != NULL)
            return alloc;
        /*
         * TODO: add debug prints for diaganostics that we didn't get free slab
         * and trying next page in the list
         */
    }

    /*
     * If we reached here that means it's time to allocate a new meta_slab_t entry
     * and a new page to hold base address
     */
    meta_slab = slab_alloc(&global_meta_slab);
    if (meta_slab == NULL) {
        dprintk("failed, not enough free pages\n");
        return NULL;
    }

    dprintk("meta_slab allocated %p\n", meta_slab);

    /*
     * for now let's allocate 4K pages for all allocations,
     * we can change the logic in future to have 2M for larger slab sizes
     */

    free_page = get_free_pages(PAGE_ORDER_4K, GFP_KERNEL);
    if (free_page == NULL) {
        dprintk("ktf_alloc failed, not enough free pages\n");
        slab_free(&global_meta_slab, meta_slab);
        return NULL;
    }
    memset(free_page, 0, PAGE_SIZE);

    meta_slab->slab_base = free_page;
    meta_slab->slab_len = PAGE_SIZE;
    meta_slab->slab_size = size_power2;
    ret = initialize_slab(meta_slab);

    if (ret != ESUCCESS) {
        dprintk("initialize_slab failed\n");
        put_pages(free_page, PAGE_ORDER_4K);
        return NULL;
    }

    list_add(&meta_slab->list, &meta_slab_list[order_index]);
    alloc = slab_alloc(meta_slab);

    return alloc;
}

/*
 * Loop through all the orders and check where does this memory belong
 * Then link it back into free list. Not a O(1) of implementation but we're looking for
 * simple now
 */
void ktf_free(void *ptr) {
    int alloc_order;
    meta_slab_t *slab = NULL;

    for (alloc_order = SLAB_ORDER_16; alloc_order < SLAB_ORDER_MAX; alloc_order++) {
        /* Go through list of meta_slab_t and try to allocate a free slab */
        list_for_each_entry (slab, &meta_slab_list[alloc_order], list) {
            if ((_ul(ptr) >= (_ul(slab->slab_base))) &&
                (_ul(ptr) < (_ul(slab->slab_base) + _ul(slab->slab_len)))) {
                slab_free(slab, ptr);
                return;
            }
        }
    }

    panic("Attempted to free %p and couldn't find it\n", ptr);
    /* If we reached here, something terribly went wrong */
    UNREACHABLE();
}

int init_slab(void) {
    int ret = 0;
    int i = 0;
    void *alloc_pages = NULL;

    printk("Initialize SLAB\n");
    memset(&meta_slab_list, 0, sizeof(meta_slab_list));
    memset(&global_meta_slab, 0, sizeof(global_meta_slab));

    alloc_pages = get_free_pages(PAGE_ORDER_4K, GFP_KERNEL);
    if (NULL == alloc_pages) {
        dprintk("get_free_pages failed\n");
        return -ENOMEM;
    }
    memset(alloc_pages, 0, PAGE_SIZE);

    global_meta_slab.slab_base = alloc_pages;
    global_meta_slab.slab_len = PAGE_SIZE;
    global_meta_slab.slab_size = next_power_of_two(sizeof(meta_slab_t));
    ret = initialize_slab(&global_meta_slab);
    if (ret != ESUCCESS) {
        dprintk("initialize_slab failed\n");
        put_pages(alloc_pages, PAGE_ORDER_4K);
        return ret;
    }

    for (i = SLAB_ORDER_16; i < SLAB_ORDER_MAX; i++) {
        list_init(&meta_slab_list[i]);
    }

    dprintk("After initializing slab module\n");
    return ret;
}
