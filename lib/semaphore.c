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

#include <mm/slab.h>
#include <semaphore.h>

void sem_init(sem_t *sem, uint32_t value) {
    /* Do not allow more than 2^31-1 threads to access this semaphore */
    BUG_ON(value > MAX_SEMAPHORE_VALUE);

    atomic_set(&(sem->v), value);
}

int32_t sem_value(const sem_t *sem) { return atomic_read(&sem->v); }

bool sem_trywait(sem_t *sem) {
    int64_t val;

    if (sem_value(sem) > 0) {
        val = atomic_dec_return(&(sem->v));
        if (val >= 0) {
            return true;
        }
        else {
            atomic_inc(&(sem->v));
        }
    }

    return false;
}

bool sem_trywait_units(sem_t *sem, int32_t units) {
    int64_t val;

    if (sem_value(sem) >= units) {
        val = atomic_sub_return(&(sem->v), units);
        if (val >= 0) {
            return true;
        }
        else {
            atomic_add_return(&(sem->v), units);
        }
    }

    return false;
}

void sem_wait(sem_t *sem) {
    while (!sem_trywait(sem)) {
        cpu_relax();
    }
}

void sem_wait_units(sem_t *sem, int32_t units) {
    while (!sem_trywait_units(sem, units)) {
        cpu_relax();
    }
}

void sem_post(sem_t *sem) { atomic_inc(&(sem->v)); }

void sem_post_units(sem_t *sem, int32_t units) { atomic_add_return(&(sem->v), units); }
