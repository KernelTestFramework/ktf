#ifndef KTF_ASM_MACROS_H
#define KTF_ASM_MACROS_H

.macro putc val
  movw $0x3f8, %dx;
  movb $\val, %al;
  outb %al, %dx;
.endm

#define GLOBAL(name) \
    .global name;     \
name:

#define SECTION(name, alignment) \
    .section name; \
    .align alignment

#endif /* KTF_ASM_MACROS_H */
