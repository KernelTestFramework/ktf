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
