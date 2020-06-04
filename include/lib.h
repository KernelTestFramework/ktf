#ifndef KTF_LIB_H
#define KTF_LIB_H

#include <ktf.h>
#include <segment.h>
#include <asm-macros.h>

static inline void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    asm volatile ("cpuid"
        : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
        : "0" (leaf)
    );
}

static inline uint32_t cpuid_eax(uint32_t leaf) {
    uint32_t eax, ign;

    cpuid(leaf, &eax, &ign, &ign, &ign);
    return eax;
}

static inline uint32_t cpuid_ebx(uint32_t leaf) {
    uint32_t ebx, ign;

    cpuid(leaf, &ign, &ebx, &ign, &ign);
    return ebx;
}

static inline uint32_t cpuid_ecx(uint32_t leaf) {
    uint32_t ecx, ign;

    cpuid(leaf, &ign, &ign, &ecx, &ign);
    return ecx;
}

static inline uint32_t cpuid_edx(uint32_t leaf) {
    uint32_t edx, ign;

    cpuid(leaf, &ign, &ign, &ign, &edx);
    return edx;
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

static inline void write_cr3(unsigned long cr3) {
    asm volatile ("mov %0, %%cr3" :: "r" (cr3));
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

static inline void write_sp(unsigned long sp) {
    asm volatile ("mov %0, %%" STR(_ASM_SP) :: "r" (sp) : "memory");
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

static inline void ud2(void) {
    asm volatile ("ud2");
}

#define BUG() do { ud2(); } while(0)
#define BUG_ON(cond) do { \
    if ((cond)) BUG();    \
} while(0)

#define ASSERT(cond) do {                       \
    if (!(cond))                                \
        panic("%s: Assert at %d failed: %s\n",  \
              __func__, STR((cond)), __LINE__); \
} while(0)

/* I/O Ports handling */

static inline uint8_t inb(io_port_t port) {
    uint8_t value;

    asm volatile ("inb %1, %0" : "=a" (value) : "Nd" (port));

    return value;
}

static inline void outb(io_port_t port, uint8_t value) {
    asm volatile ("outb %0, %1" :: "a" (value), "Nd" (port));
}

static inline void outw(io_port_t port, uint16_t value) {
    asm volatile ("outw %0, %1" :: "a" (value), "Nd" (port));
}

static inline void putc(io_port_t port, int c) {
    outb(port, c);
}

static inline void puts(io_port_t port, const char *buf, size_t len) {
    asm volatile("rep; outsb"
                 : "+S" (buf), "+c" (len)
                 : "d" (port));
}

/* I/O port delay is believed to take ~1ms */
#define IO_DELAY_PORT 0x80
static inline void io_delay(void) {
    outb(IO_DELAY_PORT, 0xff); /* Random data write */
}

/* External declarations */

extern void halt(void);

#endif /* KTF_LIB_H */
