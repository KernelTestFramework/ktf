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
#ifndef KTF_BITMAP_H
#define KTF_BITMAP_H

#include <lib.h>

#define BITS_PER_LONG        (__SIZEOF_LONG__ * 8)
#define BITS_TO_LONGS(nbits) div_round_up(nbits, BITS_PER_LONG)

typedef struct {
    unsigned long *word;
    unsigned int nbits;
} bitmap_t;

bitmap_t *bitmap_alloc(unsigned int nbits);
void bitmap_free(bitmap_t *map);

static inline unsigned int bitmap_find_first_set(const bitmap_t *map) {
    if (NULL == map || NULL == map->word)
        return UINT_MAX;

    for (unsigned int i = 0; i < BITS_TO_LONGS(map->nbits); i++) {
        unsigned int ret = __builtin_ffsl(map->word[i]);
        if (0 == ret)
            continue;

        return i * BITS_PER_LONG + ret - 1;
    }

    return UINT_MAX;
}

static inline unsigned int bitmap_find_first_clear(const bitmap_t *map) {
    if (NULL == map || NULL == map->word)
        return UINT_MAX;

    for (unsigned int i = 0; i < BITS_TO_LONGS(map->nbits); i++) {
        unsigned int ret = __builtin_ffsl(~map->word[i]);
        if (0 == ret)
            continue;

        return i * BITS_PER_LONG + ret - 1;
    }

    return UINT_MAX;
}

static inline bool bitmap_test_bit(const bitmap_t *map, unsigned int bit) {
    unsigned int i;

    if (NULL == map || NULL == map->word || bit >= map->nbits)
        return false;

    i = bit / BITS_PER_LONG;

    return (map->word[i] & (1UL << (bit % BITS_PER_LONG))) != 0;
}

static inline void bitmap_set_bit(bitmap_t *map, unsigned int bit) {
    unsigned int i;

    if (NULL == map || NULL == map->word || bit >= map->nbits)
        return;

    i = bit / BITS_PER_LONG;

    map->word[i] |= (1UL << (bit % BITS_PER_LONG));
}

static inline void bitmap_clear_bit(bitmap_t *map, unsigned int bit) {
    unsigned int i;

    if (NULL == map || NULL == map->word || bit >= map->nbits)
        return;

    i = bit / BITS_PER_LONG;

    map->word[i] &= ~(1UL << (bit % BITS_PER_LONG));
}

#endif /* KTF_BITMAP_H */
