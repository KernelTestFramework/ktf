#include <page.h>

#if defined(__x86_64__)

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

#elif defined(__i386__)

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)

#endif

ENTRY(_start)
SECTIONS
{
    . = 4K;

    .multiboot BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)
    }

    .text.init BLOCK(4K) : ALIGN(4K)
    {
        . = ALIGN(4K);
        __start_text_init = .;
            *(.text.init)
        . = ALIGN(4K);
        __end_text_init = .;
    }

    .data.init BLOCK(4K) : ALIGN(4K)
    {
        . = ALIGN(4K);
        __start_data_init = .;
            *(.data.init)
        . = ALIGN(4K);
        __end_data_init = .;
    }

    .bss.init BLOCK(4K) : ALIGN(4K)
    {
        . = ALIGN(4K);
        __start_bss_init = .;
            *(.bss.init)
            *(COMMON.init)
        . = ALIGN(4K);
        __end_bss_init = .;

    }

    . += VIRT_KERNEL_BASE;

    .text : AT(ADDR(.text) - VIRT_KERNEL_BASE)
    {
        . = ALIGN(4K);
        __start_text = .;
            *(.text)
        . = ALIGN(4K);
        __end_text = .;
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        . = ALIGN(4K);
        __start_data = .;
            *(.data)
        . = ALIGN(4K);
        __end_data = .;
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        . = ALIGN(4K);
        __start_bss = .;
            *(COMMON)
            *(.bss)
        . = ALIGN(4K);
        __end_bss = .;
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        . = ALIGN(4K);
        __start_rodata = .;
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4K);
        __end_rodata = .;
    }

    .note :
    {
        . = ALIGN(4K);
        __start_note = .;
        *(.note)
        *(.note.*)
        . = ALIGN(4K);
        __end_note = .;
    }

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.comment)
        *(.note)
    }
}
