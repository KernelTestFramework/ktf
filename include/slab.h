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
#include <page.h>
#include <list.h>

enum slab_alloc_order {
    SLAB_ORDER_8,
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
    SLAB_SIZE_MIN = 8,
    SLAB_SIZE_8 = SLAB_SIZE_MIN,
    SLAB_SIZE_16 = SLAB_SIZE_8 << 1,
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

#define SLAB_SIZE_FULL_MASK ((SLAB_SIZE_MAX<<1) - 1)

/* 
 * SLAB sizes >= 4K should directly allocate pages
 */

struct slab {
    struct slab *next_slab;
	list_head_t list;
};

typedef struct slab slab_t;

struct meta_slab {
    list_head_t list;
    slab_t *next_free_slab;
    slab_t *slab_base;
    unsigned int slab_len;
    unsigned int slab_size;
};

typedef struct meta_slab meta_slab_t;

int  init_slab(void);
extern void *ktf_alloc(unsigned int size);
extern void ktf_free(void *ptr);

#endif /* KTF_ALLOC_SLAB_H */
