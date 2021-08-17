/*
 * Copyright © 2020 Amazon.com, Inc. or its affiliates.
 * Copyright © 2014,2015 Citrix Systems Ltd.
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
#include <asm-macros.h>
#include <page.h>

#define EARLY_PT_NUM (EARLY_VIRT_MEM / 2)
#define PT_NAME(x) l1_pt_entries ## x

#if EARLY_PT_NUM > L2_PT_ENTRIES
#error "Unable to cover more than 1GB of early address space"
#endif

#define FIRST_PT_IDX 0

/* Each single l1_pt_entry covers a 4K of virtual addr range */
.macro alloc_l1_pt idx
    GLOBAL(PT_NAME(\idx))
        .rept L1_PT_ENTRIES
        .long PT_PADDR(PT_NAME(0), L1_PT_SHIFT) + L1_PROT, 0
        .endr
    END_OBJECT(PT_NAME(\idx))
.endm

.macro l2_pt_entry idx
    .long PT_NAME(\idx) + L2_PROT, 0
.endm

.altmacro

/* Initial identity map page tables */
SECTION(.data.init, "aw", PAGE_SIZE)

/* Allocate L1 page tables */
.set i, FIRST_PT_IDX
.rept EARLY_PT_NUM
    alloc_l1_pt %i
    .set i, i + 1
.endr

/* 
 * Each single l2_pt_entry points to l1_pt_entry table and 
 * thus a single l2_pt_entry covers a 2M of virtual addr range 
 */
GLOBAL(l2_pt_entries)
    /* Assign L1 page tables to their L2 table */
    .set i, FIRST_PT_IDX
    .rept EARLY_PT_NUM
        l2_pt_entry %i
        .set i, i + 1
    .endr
    .fill (L2_PT_ENTRIES - EARLY_PT_NUM), PTE_SIZE, 0      /* EARLY_PT_NUM entries used, rests all are zeroed */
END_OBJECT(l2_pt_entries)

/* 
 * Each single l3_pt_entry points to a l2_pt_entry table and 
 * thus a single l3_pt_entry covers a 1G of virtual addr range 
 */
#if defined(__i386__)
.align PAGE_SIZE
#endif
GLOBAL(l3_pt_entries)
    /* Should cover identity and user addresses */
    .long l2_pt_entries + L3_PROT, 0

    .fill (L3_PT_ENTRIES - 3), PTE_SIZE, 0

    /* 
    * Kernel range starts at 0xffffffff80000000. 
    * That makes l3 index = 0x1fe.
    */
    .long l2_pt_entries + L3_PROT, 0

    /* 
    * Don't expect to spill over 0x1fe at boot time. So 1ff is zeroed
    */
    .quad 0
END_OBJECT(l3_pt_entries)

#if defined(__x86_64__)
.align PAGE_SIZE
GLOBAL(l4_pt_entries)
    .long l3_pt_entries + L4_PROT, 0

    .fill (L4_PT_ENTRIES - 2), PTE_SIZE, 0

    /* 0xffff... */
    .long l3_pt_entries + L4_PROT, 0
END_OBJECT(l4_pt_entries)
#endif

.noaltmacro
