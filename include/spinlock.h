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
