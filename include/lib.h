#ifndef KTF_LIB_H
#define KTF_LIB_H

#include <ktf.h>
#include <segment.h>
#include <asm-macros.h>

#if defined(__i386__)
#define SPRAY_VAL 0x01010101U
#define STOS stosl
#define MOVS movsl
#elif defined (__x86_64__)
#define SPRAY_VAL 0x0101010101010101UL
#define STOS stosq
#define MOVS movsq
#endif

#define ARCH_SIZE (sizeof(void *))

static inline void *memset(void *s, int c, size_t n) {
    unsigned long d0;

    asm volatile(
        "mov %1, %%" STR(_ASM_CX) "\n"
        "rep stosb\n"
        "mov %2, %%" STR(_ASM_CX) "\n"
        "rep " STR(STOS) "\n"
    : "=&D" (d0)
    : "ir" (n & (ARCH_SIZE - 1)),
      "ir" (n / ARCH_SIZE),
      "0" (s),
      "a" (SPRAY_VAL * c)
    : STR(_ASM_CX), "memory"
    );

   return s;
}

static inline void *memcpy(void *d, void *s, size_t n) {
    unsigned long d0;

    asm volatile(
        "mov %2, %%" STR(_ASM_CX) "\n"
        "rep movsb \n"
        "mov %3, %%" STR(_ASM_CX) "\n"
        "rep " STR(MOVS) "\n"
    : "=&D" (d0), "+&S" (s)
    : "ir" (n & (ARCH_SIZE - 1)),
      "ir" (n / ARCH_SIZE),
      "0" (d)
    : STR(_ASM_CX), "memory"
    );

   return d;
}

static inline char *strcpy(char *d, const char *s) {
    int d0, d1, d2;

    asm volatile ("1: lodsb \n"
                  "   stosb \n"
                  "   testb %%al,%%al \n"
                  "   jne 1b"
    : "=&S" (d0), "=&D" (d1), "=&a" (d2)
    : "0" (s), "1" (d)
    : "memory");

    return d;
}

static inline void sti(void) {
    asm volatile ("sti");
}

static inline void cli(void) {
    asm volatile ("cli");
}

static inline void pause(void) {
    asm volatile ("pause");
}

static inline void hlt(void) {
    asm volatile ("hlt");
}

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

static inline void lgdt(const gdt_ptr_t *gdt_ptr) {
    asm volatile ("lgdt %0" :: "m" (*gdt_ptr));
}

static inline void lidt(const idt_ptr_t *idt_ptr) {
    asm volatile ("lidt %0" :: "m" (*idt_ptr));
}

static inline void ltr(unsigned int selector) {
    asm volatile ("ltr %w0" :: "rm" (selector));
}

static inline void sgdt(gdt_ptr_t *gdt_ptr) {
    asm volatile ("sgdt %0" : "=m" (*gdt_ptr));
}

static inline void sidt(idt_ptr_t *idt_ptr) {
    asm volatile ("sidt %0" : "=m" (*idt_ptr));
}

static inline void str(unsigned int *selector) {
    asm volatile ("str %0" : "=m" (*selector));
}

/* External declarations */

extern void halt(void);

#endif /* KTF_LIB_H */
