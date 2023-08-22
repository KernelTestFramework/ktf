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
#ifndef KTF_LIB_H
#define KTF_LIB_H

#include <asm-macros.h>
#include <console.h>
#include <ktf.h>
#include <processor.h>
#include <segment.h>

#define min(a, b)                                                                        \
    ({                                                                                   \
        const typeof(a) _a = (a);                                                        \
        const typeof(b) _b = (b);                                                        \
        _a < _b ? _a : _b;                                                               \
    })

#define max(a, b)                                                                        \
    ({                                                                                   \
        const typeof(a) _a = (a);                                                        \
        const typeof(b) _b = (b);                                                        \
        _a > _b ? _a : _b;                                                               \
    })

#define div_round_up(n, d) ((((n) + (d)) - 1) / (d))

static inline unsigned int log2(unsigned int value) {
    return value == 0 ? 0 : (31 - __builtin_clz(value));
}

static inline unsigned int llog2(unsigned long value) {
    return value == 0 ? 0 : (63 - __builtin_clzl(value));
}

#define sfence() asm volatile("sfence" ::: "memory")
#define lfence() asm volatile("lfence" ::: "memory")
#define mfence() asm volatile("mfence" ::: "memory")

#define mb()  mfence()
#define rmb() lfence()
#define wmb() sfence()

#define smp_mb()                                                                         \
    do {                                                                                 \
        mb();                                                                            \
        barrier();                                                                       \
    } while (0)
#define smp_rmb()                                                                        \
    do {                                                                                 \
        rmb();                                                                           \
        barrier();                                                                       \
    } while (0)
#define smp_wmb()                                                                        \
    do {                                                                                 \
        wmb();                                                                           \
        barrier();                                                                       \
    } while (0)

static inline void clflush(const volatile void *p) {
    asm volatile("clflush %0" ::"m"(*(char const *) p) : "memory");
}

static inline void prefetchw(const void *p) {
    asm volatile("prefetchw %0" ::"m"(*(char const *) p) : "memory");
}

static inline void prefetcht0(const void *p) {
    asm volatile("prefetcht0 %0" ::"m"(*(char const *) p) : "memory");
}

static inline void prefetcht1(const void *p) {
    asm volatile("prefetcht1 %0" ::"m"(*(char const *) p) : "memory");
}

static inline void prefetcht2(const void *p) {
    asm volatile("prefetcht2 %0" ::"m"(*(char const *) p) : "memory");
}

static inline void prefetchnta(const void *p) {
    asm volatile("prefetchnta %0" ::"m"(*(char const *) p) : "memory");
}

static inline void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx,
                         uint32_t *edx) {
    asm volatile("cpuid"
                 : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                 : "0"(leaf), "1"(*ebx), "2"(*ecx), "3"(*edx));
}

static inline uint32_t cpuid_eax(uint32_t leaf) {
    uint32_t eax = 0, ign = 0;

    cpuid(leaf, &eax, &ign, &ign, &ign);
    return eax;
}

static inline uint32_t cpuid_ebx(uint32_t leaf) {
    uint32_t ebx = 0, ign = 0;

    cpuid(leaf, &ign, &ebx, &ign, &ign);
    return ebx;
}

static inline uint32_t cpuid_ecx(uint32_t leaf) {
    uint32_t ecx = 0, ign = 0;

    cpuid(leaf, &ign, &ign, &ecx, &ign);
    return ecx;
}

static inline uint32_t cpuid_edx(uint32_t leaf) {
    uint32_t edx = 0, ign = 0;

    cpuid(leaf, &ign, &ign, &ign, &edx);
    return edx;
}

static inline uint64_t rdmsr(uint32_t msr_idx) {
    uint32_t low, high;

    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr_idx));

    return (((uint64_t) high) << 32) | low;
}

static inline void wrmsr(uint32_t msr_idx, uint64_t value) {
    asm volatile("wrmsr" ::"c"(msr_idx), "a"((uint32_t) value),
                 "d"((uint32_t)(value >> 32)));
}

static inline void sti(void) {
    asm volatile("sti");
}

static inline void cli(void) {
    asm volatile("cli");
}

static inline void pause(void) {
    asm volatile("pause");
}

static inline void hlt(void) {
    asm volatile("hlt");
}

static inline unsigned long read_flags(void) {
    unsigned long flags;

    asm volatile(
#if defined(__i386__)
        "pushfd;"
        "popl %0"
#elif defined(__x86_64__)
        "pushfq;"
        "popq %0"
#endif
        : "=r"(flags));

    return flags;
}

static inline unsigned long read_cs(void) {
    unsigned long cs;

    asm volatile("mov %%cs, %0" : "=r"(cs));

    return cs;
}

static inline unsigned long read_ds(void) {
    unsigned long ds;

    asm volatile("mov %%ds, %0" : "=r"(ds));

    return ds;
}

static inline unsigned long read_ss(void) {
    unsigned long ss;

    asm volatile("mov %%ss, %0" : "=r"(ss));

    return ss;
}

static inline unsigned long read_es(void) {
    unsigned long es;

    asm volatile("mov %%es, %0" : "=r"(es));

    return es;
}

static inline unsigned long read_fs(void) {
    unsigned long fs;

    asm volatile("mov %%fs, %0" : "=r"(fs));

    return fs;
}

static inline void write_fs(unsigned long fs) {
    asm volatile("mov %0, %%fs" ::"r"(fs));
}

static inline unsigned long read_gs(void) {
    unsigned long gs;

    asm volatile("mov %%gs, %0" : "=r"(gs));

    return gs;
}

static inline void write_gs(unsigned long gs) {
    asm volatile("mov %0, %%gs" ::"r"(gs));
}

static inline unsigned long read_cr0(void) {
    unsigned long cr0;

    asm volatile("mov %%cr0, %0" : "=r"(cr0));

    return cr0;
}

static inline void write_cr0(unsigned long cr0) {
    asm volatile("mov %0, %%cr0" ::"r"(cr0));
}

static inline unsigned long read_cr2(void) {
    unsigned long cr2;

    asm volatile("mov %%cr2, %0" : "=r"(cr2));

    return cr2;
}

static inline unsigned long read_cr3(void) {
    unsigned long cr3;

    asm volatile("mov %%cr3, %0" : "=r"(cr3));

    return cr3;
}

static inline void write_cr3(unsigned long cr3) {
    asm volatile("mov %0, %%cr3" ::"r"(cr3));
}

static inline unsigned long read_cr4(void) {
    unsigned long cr4;

    asm volatile("mov %%cr4, %0" : "=r"(cr4));

    return cr4;
}

static inline void write_cr4(unsigned long cr4) {
    asm volatile("mov %0, %%cr4" ::"r"(cr4));
}

static inline unsigned long read_cr8(void) {
    unsigned long cr8;

    asm volatile("mov %%cr8, %0" : "=r"(cr8));

    return cr8;
}

#define WRITE_SP(sp)                                                                     \
    do {                                                                                 \
        asm volatile("mov %0, %%" STR(_ASM_SP)::"r"((sp)) : "memory");                   \
    } while (0)

static inline void lgdt(const gdt_ptr_t *gdt_ptr) {
    asm volatile("lgdt %0" ::"m"(*gdt_ptr));
}

static inline void lidt(const idt_ptr_t *idt_ptr) {
    asm volatile("lidt %0" ::"m"(*idt_ptr));
}

static inline void ltr(unsigned int selector) {
    asm volatile("ltr %w0" ::"rm"(selector));
}

static inline void sgdt(gdt_ptr_t *gdt_ptr) {
    asm volatile("sgdt %0" : "=m"(*gdt_ptr));
}

static inline void sidt(idt_ptr_t *idt_ptr) {
    asm volatile("sidt %0" : "=m"(*idt_ptr));
}

static inline void str(unsigned int *selector) {
    asm volatile("str %0" : "=m"(*selector));
}

static inline void flush_tlb(void) {
    write_cr3(read_cr3());
}

static inline void ud2(void) {
    asm volatile("ud2");
}

static inline void swapgs(void) {
    asm volatile("swapgs");
}

static inline void sysret(void) {
#if defined(__i386__)
    asm volatile("sysret");
#else
    asm volatile("sysretq");
#endif
}

#define BUG()                                                                            \
    do {                                                                                 \
        panic("BUG in %s() at line %u\n", __func__, __LINE__);                           \
    } while (true)
#define BUG_ON(cond)                                                                     \
    do {                                                                                 \
        if ((cond))                                                                      \
            BUG();                                                                       \
    } while (0)

#define ASSERT(cond)                                                                     \
    do {                                                                                 \
        if (!(cond))                                                                     \
            panic("%s: Assert at %d failed: %s\n", __func__, __LINE__, STR((cond)));     \
    } while (0)

/* I/O Ports handling */

static inline uint8_t inb(io_port_t port) {
    uint8_t value;

    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));

    return value;
}

static inline uint16_t inw(io_port_t port) {
    uint16_t value;

    asm volatile("inw %1, %0" : "=a"(value) : "Nd"(port));

    return value;
}

static inline uint32_t ind(io_port_t port) {
    uint32_t value;

    asm volatile("in %1, %0" : "=a"(value) : "Nd"(port));

    return value;
}

static inline void outb(io_port_t port, uint8_t value) {
    asm volatile("outb %0, %1" ::"a"(value), "Nd"(port));
}

static inline void outw(io_port_t port, uint16_t value) {
    asm volatile("outw %0, %1" ::"a"(value), "Nd"(port));
}

static inline void outd(io_port_t port, uint32_t value) {
    asm volatile("out %0, %1" ::"a"(value), "Nd"(port));
}

static inline void putc(io_port_t port, int c) {
    outb(port, c);
}

static inline void puts(io_port_t port, const char *buf, size_t len) {
    asm volatile("rep; outsb" : "+S"(buf), "+c"(len) : "d"(port));
}

/* I/O port delay is believed to take ~1ms */
#define IO_DELAY_PORT 0x80
static inline void io_delay(void) {
    outb(IO_DELAY_PORT, 0xff); /* Random data write */
}

static inline uint64_t rdtsc(void) {
    unsigned int low, high;

    asm volatile("rdtsc" : "=a"(low), "=d"(high));

    return ((uint64_t) high << 32) | low;
}

static inline uint64_t rdtscp(void) {
    unsigned int low, high;

    asm volatile("rdtscp" : "=a"(low), "=d"(high)::"ecx");

    return ((uint64_t) high << 32) | low;
}

static inline void rep_nop(void) {
    asm volatile("rep;nop" ::: "memory");
}
#define cpu_relax() rep_nop()

static inline void wait_cycles(unsigned int cycles) {
    uint64_t start = rdtscp();

    while (rdtscp() - start < cycles)
        cpu_relax();
}

static inline unsigned int next_power_of_two(unsigned int n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

static inline unsigned long ipow(int base, unsigned int exp) {
    unsigned long result = 1;
    for (;;) {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}

static inline void enable_fpu(void) {
    write_cr0((read_cr0() & ~X86_CR0_EM) | X86_CR0_MP);
    write_cr4(read_cr4() | X86_CR4_OSFXSR | X86_CR4_OSXMMEXCPT);
}

/* External declarations */

extern void halt(void);

extern void srand(unsigned s);

extern int rand(void);

extern bool wrmsr_safe(uint32_t msr_idx, uint64_t value);
extern bool rdmsr_safe(uint32_t msr_idx, uint64_t *value);

#endif /* KTF_LIB_H */
