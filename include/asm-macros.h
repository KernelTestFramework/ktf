#ifndef KTF_ASM_MACROS_H
#define KTF_ASM_MACROS_H

#ifdef __ASSEMBLY__

.macro putc val
  movw $0x3f8, %dx;
  movb $\val, %al;
  outb %al, %dx;
.endm

#define GLOBAL(name) \
    .global name;    \
name:

#define SECTION(name, flags, alignment) \
    .section name, flags;               \
    .align alignment

#define SIZE(name) \
    .size name, (. - name);

#define END_FUNC(name) \
    .type name, STT_FUNC;   \
    SIZE(name)

#endif

#endif /* KTF_ASM_MACROS_H */
