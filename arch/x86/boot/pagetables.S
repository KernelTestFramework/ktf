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

#define _PADDR(table, shift) (((. - (table)) / PTE_SIZE) << (shift))

/* Initial identity map page tables */
SECTION(.data.init, "aw", PAGE_SIZE)
GLOBAL(l1_pt_entries)
    .rept L1_PT_ENTRIES
    .long _PADDR(l1_pt_entries, L1_PT_SHIFT) + L1_PROT, 0
    .endr
END_OBJECT(l1_pt_entries)

GLOBAL(l2_pt_entries)
    .long l1_pt_entries + L2_PROT, 0
    .fill (L2_PT_ENTRIES - 1), PTE_SIZE, 0
END_OBJECT(l2_pt_entries)

#if defined(__i386__)
.align PAGE_SIZE
#endif
GLOBAL(l3_pt_entries)
    .long l2_pt_entries + L3_PROT, 0

    .fill (L3_PT_ENTRIES - 3), PTE_SIZE, 0

    .long l2_pt_entries + L3_PROT, 0

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
