#include <asm-macros.h>
#include <processor.h>
#include <segment.h>

.code16
SECTION(.text.init, "ax", 0x1000)
GLOBAL(ap_start)
    mov   %cs, %ax
    mov   %ax, %ds

    lgdt boot_gdt_ptr - ap_start

    xor   %ax, %ax
    inc   %ax
    lmsw  %ax

    ljmp  $__KERN_CS32, $.Lprot_mode

.code32
.Lprot_mode:
    mov   $__KERN_DS32, %eax
    mov   %eax, %ds
    mov   %eax, %es
    mov   %eax, %gs
    mov   %eax, %fs
    mov   %eax, %ss
    mov   $_boot_stack_top, %esp
    mov   %esp, %ebp

    mov   %cr4, %eax
    or    $(X86_CR4_PAE | X86_CR4_PSE), %eax
    mov   %eax, %cr4

    mov   (cr3), %eax
    mov   %eax, %cr3

    /* Enable long mode */
    movl  $MSR_EFER, %ecx
    rdmsr
    or    $EFER_LME, %eax
    wrmsr

    /* Activate long mode: enable paging */
    mov   %cr0, %eax
    or    $(X86_CR0_PG | X86_CR0_WP), %eax
    mov   %eax, %cr0

    /* clear prefetch and jump to 64bit code */
    ljmp $__KERN_CS64, $.Llong_mode

.code64
.Llong_mode:
    xor %rax, %rax
    mov %rax, %ds
    mov %rax, %es
    mov %rax, %fs
    mov %rax, %gs
    mov %rax, %ss

    push $X86_EFLAGS_MBS
    popf

    cld

    jmp ap_startup

    ud2
