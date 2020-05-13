#include <asm-macros.h>
#include <processor.h>
#include <segment.h>

#define BOOT_STACK_SIZE 0x1000

.code32
SECTION(.text.init, "ax", 16)
GLOBAL(_start)
    mov   %cs, %ax
    mov   %ax, %ds

    lgdt  boot_gdt_ptr

    xor   %ax, %ax
    inc   %ax
    lmsw  %ax

    ljmp  $__KERN_CS32, $.Lprot_mode

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
    or    $X86_CR4_PAE, %eax
    mov   %eax, %cr4

    mov   $l4_pt_entries, %eax
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

    mov $kernel_stack + 0x1000, %rsp

    push $X86_EFLAGS_MBS
    popf

    cld
    mov %rbx, %rdi
    call kernel_start

GLOBAL(arch_crash)
    cli
.Linf_loop:
    hlt
    pause
    jmp .Linf_loop
END_FUNC(arch_crash)

SECTION(.data.init, "aw", 16)
_boot_stack:
    .skip BOOT_STACK_SIZE
_boot_stack_top:

#define XEN_ELFNOTE_PHYS32_ENTRY  18
ELF_NOTE(Xen, XEN_ELFNOTE_PHYS32_ENTRY, .long, _start)
