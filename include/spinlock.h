/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef KTF_SPINLOCK_H
#define KTF_SPINLOCK_H

#include <ktf.h>
#include <lib.h>
#include <atomic.h>

#define LOCK_BIT 0U

typedef volatile unsigned int spinlock_t;

#define SPINLOCK_INIT (0U)

/* Static declarations */

static inline void spin_lock(spinlock_t *lock) {
    ASSERT(lock);
    while(test_and_set_bit(LOCK_BIT, lock))
        cpu_relax();
}

static inline void spin_unlock(spinlock_t *lock) {
    ASSERT(lock);
    test_and_reset_bit(LOCK_BIT, lock);
}

/* External declarations */

#endif /* KTF_SPINLOCK_H */
