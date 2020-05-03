ENTRY(_start)
SECTIONS
{
    . = 8K;

    .multiboot BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)
    }

    . = 1M;

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
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
