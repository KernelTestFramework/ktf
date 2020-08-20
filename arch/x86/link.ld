/*
 * Copyright © 2020 Amazon.com, Inc. or its affiliates.
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <page.h>

#if defined(__x86_64__)

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

#elif defined(__i386__)

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)

#endif

PHDRS
{
    init PT_LOAD;
    user PT_LOAD AT(1M);
    kernel PT_LOAD;
    note PT_NOTE;
}

ENTRY(_start)
SECTIONS
{
    .multiboot BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)
    } :init

    .note :
    {
        . = ALIGN(4K);
        __start_note = .;
        *(.note)
        *(.note.*)
        __end_note = .;
    } :init :note

    .rmode BLOCK(4K) : ALIGN(4K)
    {
	. = ALIGN(4K);
	__start_rmode = .;
	*(.real_prot_tramp)
	*(.rlm_save_area)
	. = ALIGN(4K);
	__end_rmode = .;
    } :init

    .text.init BLOCK(4K) : ALIGN(4K)
    {
        . = ALIGN(4K);
        __start_text_init = .;
            *(.text.init)
        . = ALIGN(4K);
        __end_text_init = .;
    } :init

    .data.init BLOCK(4K) : ALIGN(4K)
    {
        . = ALIGN(4K);
        __start_data_init = .;
            *(.data.init)
        . = ALIGN(4K);
        __end_data_init = .;
    } :init

    .bss.init BLOCK(4K) : ALIGN(4K)
    {
        . = ALIGN(4K);
        __start_bss_init = .;
            *(.bss.init)
            *(COMMON.init)
        . = ALIGN(4K);
        __end_bss_init = .;
    } :init

    . = VIRT_USER_BASE + 1M;

    .text.user : AT(ADDR(.text.user) - VIRT_USER_BASE)
    {
        . = ALIGN(4K);
        __start_text_user = .;
            *(.text.user)
        . = ALIGN(4K);
        __end_text_user = .;
    } :user

    .data.user BLOCK(4K) : ALIGN(4K)
    {
        . = ALIGN(4K);
        __start_data_user = .;
            *(.data.user)
        . = ALIGN(4K);
        __end_data_user = .;
    } :user

    .bss.user BLOCK(4K) : ALIGN(4K)
    {
        . = ALIGN(4K);
        __start_bss_user = .;
            *(.bss.user)
            *(COMMON.user)
        . = ALIGN(4K);
        __end_bss_user = .;
    } :user

    . += VIRT_KERNEL_BASE - VIRT_USER_BASE;

    .text : AT(ADDR(.text) - VIRT_KERNEL_BASE)
    {
        . = ALIGN(4K);
        __start_text = .;
            *(.text)
        . = ALIGN(4K);
        __end_text = .;
    } :kernel

    .data BLOCK(4K) : ALIGN(4K)
    {
        . = ALIGN(4K);
        __start_data = .;
            *(.data)
        __start_cmdline = .;
            *(.cmdline)
        __end_cmdline = .;
        . = ALIGN(4K);
        __end_data = .;
    } :kernel

    .bss BLOCK(4K) : ALIGN(4K)
    {
        . = ALIGN(4K);
        __start_bss = .;
            *(COMMON)
            *(.bss)
        . = ALIGN(4K);
        __end_bss = .;
    } :kernel

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        . = ALIGN(4K);
        __start_rodata = .;
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4K);
        __end_rodata = .;
    } :kernel

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.comment)
        *(.note)
    }
}
