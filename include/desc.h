#ifndef KTF_DESC_H
#define KTF_DESC_H

#include <compiler.h>
#define GDT_NULL      0x0
#define GDT_KERN_CS32 0x1
#define GDT_KERN_DS32 0x2
#define GDT_KERN_CS64 0x3

#define __KERN_CS32 (GDT_KERN_CS32 << 3)
#define __KERN_DS32 (GDT_KERN_DS32 << 3)
#define __KERN_CS64 (GDT_KERN_CS64 << 3)

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

struct gdt_ptr {
        uint16_t size;
        uint64_t addr;
} __packed;
typedef struct gdt_ptr gdt_ptr_t;

#endif

#endif /* KTF_DESC_H */
