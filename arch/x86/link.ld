ENTRY(_start)
SECTIONS
{
    . = 4K;

    .multiboot BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)
    }

    . = 640K;

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    .note :
    {
        *(.note)
        *(.note.*)
    }
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}
