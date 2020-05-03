#ifndef KTF_DESC_H
#define KTF_DESC_H

#define GDT_NULL      0x0
#define GDT_KERN_CS32 0x1
#define GDT_KERN_DS32 0x2
#define GDT_KERN_CS64 0x3

#define __KERN_CS32 (GDT_KERN_CS32 << 3)
#define __KERN_DS32 (GDT_KERN_DS32 << 3)
#define __KERN_CS64 (GDT_KERN_CS64 << 3)

#define GDT_ENTRY(flags, base, limit)                \
        ((((base)  & _U64(0xff000000)) << (56-24)) | \
         (((flags) & _U64(0x0000f0ff)) << 40)      | \
         (((limit) & _U64(0x000f0000)) << (48-16)) | \
         (((base)  & _U64(0x00ffffff)) << 16)      | \
         (((limit) & _U64(0x0000ffff))))

#ifndef __ASSEMBLY__

typedef uint64_t gdtdesc_t;

struct gdt_ptr {
        uint16_t size;
        uint64_t addr;
} __packed;
typedef struct gdt_ptr gdt_ptr_t;

#endif

#endif /* KTF_DESC_H */
