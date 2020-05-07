#ifndef KTF_LIB_H
#define KTF_LIB_H

#include <ktf.h>

static inline unsigned long read_flags(void) {
    unsigned long flags;

#if defined(__i386__)
    asm volatile ("pushfd; popl %0" : "=r" (flags));
#elif defined(__x86_64__)
    asm volatile ("pushfq; popq %0" : "=r" (flags));
#endif

    return flags;
}

static inline unsigned long read_cs(void) {
    unsigned long cs;

    asm volatile ("mov %%cs, %0" : "=r" (cs));

    return cs;
}

static inline unsigned long read_ds(void) {
    unsigned long ds;

    asm volatile ("mov %%ds, %0" : "=r" (ds));

    return ds;
}

static inline unsigned long read_ss(void) {
    unsigned long ss;

    asm volatile ("mov %%ss, %0" : "=r" (ss));

    return ss;
}

static inline unsigned long read_es(void) {
    unsigned long es;

    asm volatile ("mov %%es, %0" : "=r" (es));

    return es;
}

static inline unsigned long read_fs(void) {
    unsigned long fs;

    asm volatile ("mov %%fs, %0" : "=r" (fs));

    return fs;
}

static inline unsigned long read_gs(void) {
    unsigned long gs;

    asm volatile ("mov %%gs, %0" : "=r" (gs));

    return gs;
}

static inline unsigned long read_cr0(void) {
    unsigned long cr0;

    asm volatile ("mov %%cr0, %0" : "=r" (cr0));

    return cr0;
}

static inline unsigned long read_cr2(void) {
    unsigned long cr2;

    asm volatile ("mov %%cr2, %0" : "=r" (cr2));

    return cr2;
}

static inline unsigned long read_cr3(void) {
    unsigned long cr3;

    asm volatile ("mov %%cr3, %0" : "=r" (cr3));

    return cr3;
}

static inline unsigned long read_cr4(void) {
    unsigned long cr4;

    asm volatile ("mov %%cr4, %0" : "=r" (cr4));

    return cr4;
}

static inline unsigned long read_cr8(void) {
    unsigned long cr8;

    asm volatile ("mov %%cr8, %0" : "=r" (cr8));

    return cr8;
}

#endif /* KTF_LIB_H */
