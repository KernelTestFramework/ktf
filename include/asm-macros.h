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

#define ELF_NOTE(name, type, size, addr) \
    .section .note.name, "a";            \
    .align 4;                            \
    .long 2f - 1f; /* namesz */          \
    .long 4f - 3f;  /* descsz */         \
    .long type;    /* type   */          \
1:.asciz #name;    /* name   */          \
2:.align 4;                              \
3:size addr;       /* desc   */          \
4:.align 4;

#endif

#endif /* KTF_ASM_MACROS_H */
