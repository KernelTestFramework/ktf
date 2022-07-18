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
#ifndef KTF_ATOMIC_H
#define KTF_ATOMIC_H

#include <ktf.h>
#include <lib.h>

typedef struct {
    int32_t counter;
} atomic_t;

typedef struct {
    int64_t counter;
} atomic64_t;

#define atomic_set(v, i) (ACCESS_ONCE((v)->counter) = (i))
#define atomic_read(v)   (ACCESS_ONCE((v)->counter))

/* Static declarations */

static inline bool atomic_test_bit(unsigned int bit, volatile void *addr) {
    bool status;

    asm volatile("bt %[bit], %[addr];"
                 "setc %[status];"
                 : [ status ] "=r"(status)
                 : [ bit ] "Ir"(bit), [ addr ] "m"(*(uint8_t *) addr)
                 : "cc", "memory");

    return status;
}

static inline bool atomic_test_and_set_bit(unsigned int bit, volatile void *addr) {
    bool status;

    asm volatile("lock btsl %[bit], %[addr];"
                 "setc %[status];"
                 : [ status ] "=r"(status)
                 : [ bit ] "Ir"(bit), [ addr ] "m"(*(uint8_t *) addr)
                 : "cc", "memory");

    return status;
}

static inline bool atomic_test_and_reset_bit(unsigned int bit, volatile void *addr) {
    bool status;

    asm volatile("lock btrl %[bit], %[addr];"
                 "setc %[status];"
                 : [ status ] "=r"(status)
                 : [ bit ] "Ir"(bit), [ addr ] "m"(*(uint8_t *) addr)
                 : "cc", "memory");

    return status;
}

static inline bool atomic_test_and_complement_bit(unsigned int bit, volatile void *addr) {
    bool status;

    asm volatile("lock btc %[bit], %[addr];"
                 "setc %[status];"
                 : [ status ] "=r"(status)
                 : [ bit ] "Ir"(bit), [ addr ] "m"(*(uint8_t *) addr)
                 : "cc", "memory");

    return status;
}

static inline void atomic_inc(atomic_t *v) {
    asm volatile("lock incl %[addr];" : : [ addr ] "m"(v->counter) : "memory");
}

static inline void atomic64_inc(atomic64_t *v) {
    asm volatile("lock incq %[addr];" : : [ addr ] "m"(v->counter) : "memory");
}

static inline int32_t atomic_add_return(atomic_t *v, int32_t n) {
    int32_t val = n;
    asm volatile("lock xaddl %[n], %[addr];"
                 : [ n ] "+r"(val), [ addr ] "+m"(v->counter)
                 :
                 : "memory");
    return val;
}

static inline int64_t atomic64_add_return(atomic64_t *v, int64_t n) {
    int64_t val = n;
    asm volatile("lock xaddq %[n], %[addr];"
                 : [ n ] "+r"(val), [ addr ] "+m"(v->counter)
                 :
                 : "memory");
    return val;
}

static inline int32_t atomic_inc_return(atomic_t *v) {
    return atomic_add_return(v, 1);
}

static inline int64_t atomic64_inc_return(atomic64_t *v) {
    return atomic64_add_return(v, 1);
}

static inline int atomic_inc_and_test(atomic_t *v) {
    uint8_t c;
    asm volatile("lock incl %[addr]; sete %[c]"
                 : [ addr ] "=m"(v->counter), [ c ] "=r"(c)
                 :
                 : "memory");
    return c != 0;
}

static inline int atomic64_inc_and_test(atomic64_t *v) {
    uint8_t c;
    asm volatile("lock incq %[addr]; sete %[c]"
                 : [ addr ] "=m"(v->counter), [ c ] "=r"(c)
                 :
                 : "memory");
    return c != 0;
}

static inline void atomic_dec(atomic_t *v) {
    asm volatile("lock decl %[addr];" : : [ addr ] "m"(v->counter) : "memory");
}

static inline void atomic64_dec(atomic64_t *v) {
    asm volatile("lock decq %[addr];" : : [ addr ] "m"(v->counter) : "memory");
}

static inline int32_t atomic_sub_return(atomic_t *v, int32_t n) {
    return atomic_add_return(v, -n);
}

static inline int64_t atomic64_sub_return(atomic64_t *v, int64_t n) {
    return atomic64_add_return(v, -n);
}

static inline int32_t atomic_dec_return(atomic_t *v) {
    return atomic_sub_return(v, 1);
}

static inline int64_t atomic64_dec_return(atomic64_t *v) {
    return atomic64_sub_return(v, 1);
}

static inline int atomic_dec_and_test(atomic_t *v) {
    uint8_t c;
    asm volatile("lock decl %[addr]; sete %[c]"
                 : [ addr ] "=m"(v->counter), [ c ] "=r"(c)
                 :
                 : "memory");
    return c != 0;
}

static inline int atomic64_dec_and_test(atomic64_t *v) {
    uint8_t c;
    asm volatile("lock decq %[addr]; sete %[c]"
                 : [ addr ] "=m"(v->counter), [ c ] "=r"(c)
                 :
                 : "memory");
    return c != 0;
}

/* External declarations */

#endif /* KTF_ATOMIC_H */
