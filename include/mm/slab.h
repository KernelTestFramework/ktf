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
#ifndef KTF_ALLOC_SLAB_H
#define KTF_ALLOC_SLAB_H

#include <ktf.h>
#include <lib.h>
#include <list.h>
#include <page.h>

enum slab_alloc_order {
    SLAB_ORDER_16,
    SLAB_ORDER_32,
    SLAB_ORDER_64,
    SLAB_ORDER_128,
    SLAB_ORDER_256,
    SLAB_ORDER_512,
    SLAB_ORDER_1024,
    SLAB_ORDER_2048,
    SLAB_ORDER_MAX,
};
typedef enum slab_alloc_order slab_alloc_order_t;

enum slab_size {
    SLAB_SIZE_16 = 16,
    SLAB_SIZE_MIN = SLAB_SIZE_16,
    SLAB_SIZE_32 = SLAB_SIZE_16 << 1,
    SLAB_SIZE_64 = SLAB_SIZE_32 << 1,
    SLAB_SIZE_128 = SLAB_SIZE_64 << 1,
    SLAB_SIZE_256 = SLAB_SIZE_128 << 1,
    SLAB_SIZE_512 = SLAB_SIZE_256 << 1,
    SLAB_SIZE_1024 = SLAB_SIZE_512 << 1,
    SLAB_SIZE_2048 = SLAB_SIZE_1024 << 1,
    SLAB_SIZE_MAX = SLAB_SIZE_2048,
};
typedef enum slab_size slab_size_t;

#define SLAB_SIZE_FULL_MASK ((SLAB_SIZE_MAX << 1) - 1)

#define MAX_SLAB_ALLOC_COUNT (PAGE_SIZE / SLAB_SIZE_MIN)

#define META_SLAB_PAGE_ENTRY(meta_slab) ((meta_slab_t *) (_ul(meta_slab) & PAGE_MASK))

/*
 * SLAB sizes >= 4K should directly allocate pages
 */

struct slab {
    list_head_t list;
};

typedef struct slab slab_t;

struct meta_slab {
    list_head_t list;
    list_head_t slab_head;
    void *slab_base;
    unsigned int slab_len;
    /*
     * Don't need more than 12 bits. Currently max slab size is 2048 bytes = 2^11
     */
    unsigned int slab_size : 12;
    /*
     * slab_allocs is tracking number of allocations currently in this slab.
     * At max this can go 4096/16 = 256 slabs. Thus 10 bits are enough
     */
    unsigned int slab_allocs : 10;
    unsigned int reserved : 10;
};

typedef struct meta_slab meta_slab_t;

static inline void increment_slab_allocs(meta_slab_t *slab) {
    BUG_ON(slab == NULL);
    BUG_ON((slab->slab_allocs >= (slab->slab_len / slab->slab_size)));

    slab->slab_allocs++;
}

static inline void decrement_slab_allocs(meta_slab_t *slab) {
    BUG_ON(slab == NULL);
    BUG_ON((slab->slab_allocs == 0));

    slab->slab_allocs--;
}

static inline bool slab_is_empty(meta_slab_t *slab) {
    BUG_ON(slab == NULL);
    return (slab->slab_allocs == 0);
}

int init_slab(void);
extern void *kmalloc(size_t size);
extern void *kzalloc(size_t size);
extern void kfree(void *ptr);

#endif /* KTF_ALLOC_SLAB_H */
