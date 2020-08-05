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
#ifndef KTF_ATOMIC_H
#define KTF_ATOMIC_H

#include <ktf.h>
#include <lib.h>

/* Static declarations */

static inline bool test_bit(unsigned int bit, volatile void *addr) {
    bool status;

    asm volatile (
        "bt %[bit], %[addr];"
        "setc %[status];"
        : [status] "=r" (status)
        : [bit] "Ir" (bit), [addr] "m" (* (uint8_t *) addr)
        : "cc", "memory"
    );

    return status;
}

static inline bool test_and_set_bit(unsigned int bit, volatile void *addr) {
    bool status;

    asm volatile (
        "lock bts %[bit], %[addr];"
        "setc %[status];"
        : [status] "=r" (status)
        : [bit] "Ir" (bit), [addr] "m" (* (uint8_t *) addr)
        : "cc", "memory"
    );

    return status;
}

static inline bool test_and_reset_bit(unsigned int bit, volatile void *addr) {
    bool status;

    asm volatile (
        "lock btr %[bit], %[addr];"
        "setc %[status];"
        : [status] "=r" (status)
        : [bit] "Ir" (bit), [addr] "m" (* (uint8_t *) addr)
        : "cc", "memory"
    );

    return status;
}

static inline bool test_and_complement_bit(unsigned int bit, volatile void *addr) {
    bool status;

    asm volatile (
        "lock btc %[bit], %[addr];"
        "setc %[status];"
        : [status] "=r" (status)
        : [bit] "Ir" (bit), [addr] "m" (* (uint8_t *) addr)
        : "cc", "memory"
    );

    return status;
}

/* External declarations */

#endif /* KTF_ATOMIC_H */
