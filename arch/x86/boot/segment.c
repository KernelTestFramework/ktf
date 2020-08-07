/*
 * Copyright (c) 2020 Amazon.com, Inc. or its affiliates.
 * Copyright (c) 2014,2015 Citrix Systems Ltd.
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
