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
#include <ktf.h>
#include <lib.h>

#include <toolkit/cache/lib.h>

/* Calculate average baseline for the specified cache line timing access.
 * Calculate first intermediate average without flushing and then second
 * intermediate average based on samples collected right after flushing
 * the cache line.
 * Return average of both averages.
 */
static uint64_t test_timings(const cache_line_t *cl) {
    int i, t[SAMPLES_COUNT], avg[2];

    for (i = 0; i < SAMPLES_COUNT; i++) {
        *(volatile uint8_t *) &cl->m8[0];
        t[i] = cache_read_access_time(cl);
    }

    avg[0] = 0;
    for (i = 0; i < SAMPLES_COUNT; i++)
        avg[0] += t[i];

    for (i = 0; i < SAMPLES_COUNT; i++) {
        clflush(&cl->m8[0]);
        t[i] = cache_read_access_time(cl);
    }

    avg[1] = 0;
    for (i = 0; i < SAMPLES_COUNT; i++)
        avg[1] += t[i];

    return (avg[0] + avg[1]) / (SAMPLES_COUNT * ARRAY_SIZE(avg));
}

/*
 * Get cache access time baseline for specified memory address.
 * The function returns a number of cycles it takes to read given
 * memory on average.
 */
uint64_t cache_channel_baseline(const cache_line_t *cl, unsigned delay) {
    uint64_t baseline = ~0UL;

    for (int i = 0; i < SAMPLES_COUNT; i++) {
        baseline = min(baseline, test_timings(cl));
        wait_cycles(delay);
    }

    return baseline;
}
