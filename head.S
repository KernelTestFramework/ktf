#include <asm-macros.h>

#define __BOOT_KERN_CS 0x08
#define __BOOT_KERN_DS 0x10

#define BOOT_STACK_SIZE 0x1000

.code32
SECTION(.text, "ax", 16)

GLOBAL(gdt_desc_ptr)
    .word boot_gdt_end - boot_gdt - 1
    .long boot_gdt

boot_gdt:
    /* NULL selector entry */
    .quad 0x0

    /* [__BOOT_KERN_CS] = INIT_GDTE_SYM(0, 0xfffff, COMMON, CODE, DPL0, R, D) */
    .quad 0x00cf9a000000ffff

    /* [__BOOT_KERN_DS] = INIT_GDTE_SYM(0, 0xfffff, COMMON, DATA, DPL0, B, W) */
    .quad 0x00cf92000000ffff
boot_gdt_end:

/* ------------------------------------------------------------------------- */

.align 16
GLOBAL(_start)
    mov %cs, %ax
    mov %ax, %ds

    lgdt (gdt_desc_ptr)
    xor  %ax, %ax
    inc  %ax
    lmsw %ax

    ljmpl $__BOOT_KERN_CS, $1f
1:
    mov   $__BOOT_KERN_DS, %eax
    mov   %eax, %ds
    mov   %eax, %es
    mov   %eax, %ss
    movl  $_boot_stack_top, %esp
    movl  %esp, %ebp

    //mov  %ebx, %edi
    call kernel_start

    cli
inf_loop:
    hlt
    jmp inf_loop

SECTION(.bss, "aw", 16)
_boot_stack:
  .skip BOOT_STACK_SIZE
_boot_stack_top:

