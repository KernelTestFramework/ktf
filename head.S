#include <asm-macros.h>
#include <desc.h>

#define BOOT_STACK_SIZE 0x1000

.code32
SECTION(.text, "ax", 16)
GLOBAL(_start)
    mov   %cs, %ax
    mov   %ax, %ds

    lgdt  gdt_ptr

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





    //mov  %ebx, %edi
    call kernel_start

GLOBAL(arch_crash)
    cli
.Linf_loop:
    hlt
    pause
    jmp .Linf_loop
END_FUNC(arch_crash)

SECTION(.bss, "aw", 16)
_boot_stack:
  .skip BOOT_STACK_SIZE
_boot_stack_top:

#define XEN_ELFNOTE_PHYS32_ENTRY  18
ELF_NOTE(Xen, XEN_ELFNOTE_PHYS32_ENTRY, .long, _start)
