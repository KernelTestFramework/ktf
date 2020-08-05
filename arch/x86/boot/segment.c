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
#include <ktf.h>
#include <asm-macros.h>
#include <segment.h>

gdt_desc_t boot_gdt[] __aligned(16) __data_init = {
    [GDT_NULL].desc      = GDT_ENTRY(0x0, 0x0, 0x0),
    [GDT_KERN_CS32].desc = GDT_ENTRY(DESC_FLAGS(GR, SZ, P, DPL0, S, CODE, R, A), 0x0, 0xfffff),
    [GDT_KERN_DS32].desc = GDT_ENTRY(DESC_FLAGS(GR, SZ, P, DPL0, S, DATA, W, A), 0x0, 0xfffff),
    [GDT_KERN_CS64].desc = GDT_ENTRY(DESC_FLAGS(GR,  L, P, DPL0, S, CODE, R, A), 0x0, 0x00000),
};

gdt_ptr_t boot_gdt_ptr __data_init = {
    .size = sizeof(boot_gdt) - 1,
    .addr = _ul(&boot_gdt),
};
