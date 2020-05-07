#ifndef KTF_DESC_H
#define KTF_DESC_H

#include <compiler.h>
#include <tss.h>

#define GDT_NULL      0x0
#define GDT_KERN_CS32 0x1
#define GDT_KERN_DS32 0x2
#define GDT_KERN_CS64 0x3

#define GDT_TSS 0x4
#define __KERN_CS32 (GDT_KERN_CS32 << 3)
#define __KERN_DS32 (GDT_KERN_DS32 << 3)
#define __KERN_CS64 (GDT_KERN_CS64 << 3)

#if defined(__i386__)
#define __KERN_CS __KERN_CS32
#define __KERN_DS __KERN_DS32
#else
#define __KERN_CS __KERN_CS64
#define __KERN_DS (0)
#endif

#define GDT_ENTRY(flags, base, limit)  { .quad =     \
        ((((base)  & _U64(0xff000000)) << (56-24)) | \
         (((flags) & _U64(0x0000f0ff)) << 40)      | \
         (((limit) & _U64(0x000f0000)) << (48-16)) | \
         (((base)  & _U64(0x00ffffff)) << 16)      | \
         (((limit) & _U64(0x0000ffff)))) }

#ifndef __ASSEMBLY__

struct __packed x86_segment_desc {
    union {
        uint64_t quad;
        struct {
            uint32_t lo, hi;
        };
        struct __packed {
            uint16_t limit_lo;
            uint16_t base_lo;
            uint8_t base_mi;
            struct __packed {
                uint8_t A:1, RW:1, DC:1, E:1, S:1, DPL:2, P:1;
            };
            struct __packed {
                uint8_t limit_hi:4;
                uint8_t :1, L:1, SZ:1, GR:1;
	    };
            uint8_t base_hi;
	};

    };
};
typedef struct x86_segment_desc x86_segment_desc_t;

#define DESC_FLAG_GR     0x8000 /* Granularity of limit (0 = 1, 1 = 4K) */
#define DESC_FLAG_SZ     0x4000 /* Default operand size (0 = 16bit, 1 = 32bit) */
#define DESC_FLAG_B      0x4000 /* 'Big' flag. */
#define DESC_FLAG_L      0x2000 /* Long segment? (1 = 64bit) */
#define DESC_FLAG_AVL    0x1000 /* Available for software use */
#define DESC_FLAG_P      0x0080 /* Present? */
#define DESC_FLAG_DPL3   0x0060 /* Descriptor privilege level 3 */
#define DESC_FLAG_DPL2   0x0040 /* Descriptor privilege level 2 */
#define DESC_FLAG_DPL1   0x0020 /* Descriptor privilege level 1 */
#define DESC_FLAG_DPL0   0x0000 /* Descriptor privilege level 0 */
#define DESC_FLAG_S      0x0010 /* !System desc (0 = system, 1 = user) */
#define DESC_FLAG_CODE   0x0008 /* Type (0 = data, 1 = code)    */
#define DESC_FLAG_DATA   0x0000 /* Type (0 = data, 1 = code)    */
#define DESC_FLAG_C      0x0004 /* Conforming? (0 = non, 1 = conforming) */
#define DESC_FLAG_D      0x0004 /* Expand-down? (0 = normal, 1 = expand-down) */
#define DESC_FLAG_R      0x0002 /* Readable? (0 = XO seg, 1 = RX seg) */
#define DESC_FLAG_W      0x0002 /* Writable? (0 = RO seg, 1 = RW seg) */
#define DESC_FLAG_A      0x0001 /* Accessed? (set by hardware) */

#define DESC_FLAGS(...) (TOKEN_OR(DESC_FLAG_, ##__VA_ARGS__))

typedef x86_segment_desc_t gdtdesc_t;

static inline void set_desc_base(x86_segment_desc_t *desc, unsigned long base) {
    desc[0].base_lo =  (base & _ul(0x0000ffff));
    desc[0].base_mi = ((base & _ul(0x00ff0000)) >> 16);
    desc[0].base_hi = ((base & _ul(0xff000000)) >> 24);
#if defined(__x86_64__)
    desc[1].lo = (base >> 32);
    desc[1].hi = 0;
#endif
}

struct desc_ptr {
    uint16_t size;
#if defined(__i386__)
    uint32_t addr;
#else
    uint64_t addr;
#endif
} __packed;

typedef struct desc_ptr gdt_ptr_t;
typedef struct desc_ptr idt_ptr_t;

/*
 * Protected mode IDT entry, GDT task/call gate (8-byte)
 */
struct __packed x86_gate32 {
    union {
        struct {
            uint32_t lo, hi;
        };
        struct {
            uint16_t offset_lo;
            uint16_t selector;
            uint8_t rsvd;
            unsigned int type:4, s:1, dpl:2, p:1;
            uint16_t offset_hi;
        };
    };
};

static inline void set_gate32_offset(struct x86_gate32 *gate, unsigned long offset) {
    gate->offset_lo = (offset & _ul(0x0000ffff));
    gate->offset_hi = ((offset & _ul(0xffff0000)) >> 16);
}

/*
 * Long mode IDT entry, GDT call gate (16-byte)
 */
struct __packed x86_gate64 {
    union {
        struct {
            uint64_t lo, hi;
        };
        struct {
            uint16_t offset_lo;
            uint16_t selector;
            unsigned int ist:3, rsvd0:5;
            unsigned int type:4, s:1, dpl:2, p:1;
            uint16_t offset_mi;
            uint32_t offset_hi;
            uint32_t rsvd1;
        };
    };
};

static inline void set_gate64_offset(struct x86_gate64 *gate, unsigned long offset) {
    gate->offset_lo =  (offset & _ul(0x000000000000ffff));
    gate->offset_mi = ((offset & _ul(0x00000000ffff0000)) >> 16);
    gate->offset_hi = ((offset & _ul(0xffffffff00000000)) >> 32);
}

#define GATE_NOT_PRESENT 0x00
#define GATE_PRESENT 0x01
#define GATE_DPL0 0x00
#define GATE_DPL3 0x03

#define GATE_TYPE_INTR _U32(0xE)
#define GATE_TYPE_TRAP _U32(0xF)
#define GATE_TYPE_TASK _U32(0x5)

#define _GATE_ENTRY32(type, selector, offset, dpl, present) { \
    .offset_lo = ((offset) & _U32(0x0000ffff)),               \
    .offset_hi = (((offset) & _U32(0xffff0000)) >> 16),       \
    .selector =  ((selector) & _U32(0xffff)),                 \
    .rsvd = 0, .s = 0, .type = ((type) & _U32(0xf)),          \
    .dpl = ((dpl) & _U32(0x3)), .p = ((present) & 0x1) }

#define INTR_GATE32(selector, offset, dpl, present) _GATE_ENTRY32(GATE_TYPE_INTR, selector, offset, dpl, present)
#define TRAP_GATE32(selector, offset, dpl, present) _GATE_ENTRY32(GATE_TYPE_TRAP, selector, offset, dpl, present)
#define TASK_GATE32(selector, dpl, present) _GATE_ENTRY32(GATE_TYPE_TASK, selector, _U32(0x0), dpl, present)

#define _GATE_ENTRY64(_type, _selector, _offset, _dpl, _present) { \
    .offset_lo = (((_offset) & _U64(0x000000000000ffff))),         \
    .offset_mi = (((_offset) & _U64(0x00000000ffff0000)) >> 16),   \
    .offset_hi = (((_offset) & _U64(0xffffffff00000000)) >> 32),   \
    .selector = ((_selector) & _U32(0xffff)),                      \
    .rsvd0 = 0, .rsvd1 = 0, .s = 0, .type = ((_type) & _U32(0xf)), \
    .dpl = ((_dpl) & _U32(0x3)), .p = ((_present) & 0x1) }

#define INTR_GATE64(selector, offset, dpl, present) _GATE_ENTRY64(GATE_TYPE_INTR, selector, offset, dpl, present)
#define TRAP_GATE64(selector, offset, dpl, present) _GATE_ENTRY64(GATE_TYPE_TRAPf, selector, offset, dpl, present)

#if defined(__i386__)

typedef struct x86_gate32 task_gate_t;
typedef struct x86_gate32 idt_entry_t;

#define INTR_GATE(selector, offset, dpl, present) INTR_GATE32(selector, offset, GATE_ ## dpl, GATE_ ## present)
#define TRAP_GATE(selector, offset, dpl, present) TRAP_FATE32(selector, offset, GATE_ ## dpl, GATE_ ## present)
#define TASK_GATE(selector, dpl, present) TASK_GATE32(selector, GATE_ ## dpl, GATE_ ## present)

#define set_gate_offset(gate, offset) set_gate32_offset((gate), (offset))

#else

typedef struct x86_gate64 task_gate_t;
typedef struct x86_gate64 idt_entry_t;

#define INTR_GATE(selector, offset, dpl, present) INTR_GATE64(selector, offset, GATE_ ## dpl, GATE_ ## present)
#define TRAP_GATE(selector, offset, dpl, present) TRAP_FATE64(selector, offset, GATE_ ## dpl, GATE_ ## present)

#define set_gate_offset(gate, offset) set_gate64_offset((gate), (offset))

#endif

extern x86_tss_t tss;
extern gdtdesc_t gdt[];

extern idt_entry_t idt[256];
extern idt_ptr_t idt_ptr;
#endif

#endif /* KTF_DESC_H */
