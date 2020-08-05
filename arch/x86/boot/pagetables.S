/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <asm-macros.h>
#include <page.h>

#define _PADDR(table, shift) (((. - (table)) / PTE_SIZE) << (shift))

/* Initial identity map page tables */
SECTION(.data.init, "aw", 0x1000)
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
.align 0x1000
#endif
GLOBAL(l3_pt_entries)
    .long l2_pt_entries + L3_PROT, 0

    .fill (L3_PT_ENTRIES - 3), PTE_SIZE, 0

    .long l2_pt_entries + L3_PROT, 0

    .quad 0
END_OBJECT(l3_pt_entries)

#if defined(__x86_64__)
.align 0x1000
GLOBAL(l4_pt_entries)
    .long l3_pt_entries + L4_PROT, 0

    .fill (L4_PT_ENTRIES - 2), PTE_SIZE, 0

    /* 0xffff... */
    .long l3_pt_entries + L4_PROT, 0
END_OBJECT(l4_pt_entries)
#endif
