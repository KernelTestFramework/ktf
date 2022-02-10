/*
 * Copyright © 2020 Amazon.com, Inc. or its affiliates.
 * Copyright © 2022 Open Source Security, Inc.
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
#include <multiboot.h>

SECTION(.multiboot, "a", 4)
#define MULTIBOOT_HEADER_FLAGS (MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_VIDEO_MODE)
#define MULTIBOOT_CHECKSUM -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

#define MULTIBOOT_VIDEO_MODE_GFX 0
#define MULTIBOOT_VIDEO_MODE_TEXT 1

#define MULTIBOOT_MODE_TYPE MULTIBOOT_VIDEO_MODE_TEXT

#define MULTIBOOT_HEADER_ADDR      0
#define MULTIBOOT_LOAD_ADDRESS     0
#define MULTIBOOT_LOAD_END_ADDRESS 0
#define MULTIBOOT_BSS_END_ADDRESS  0
#define MULTIBOOT_ENTRY_ADDRESS    0

#define MULTIBOOT_FB_WIDTH  1024
#define MULTIBOOT_FB_HEIGHT  768
#define MULTIBOOT_FB_BPP      32

.code32
multiboot_header:
    .long MULTIBOOT_HEADER_MAGIC
    .long MULTIBOOT_HEADER_FLAGS
    .long MULTIBOOT_CHECKSUM
    .long MULTIBOOT_HEADER_ADDR
    .long MULTIBOOT_LOAD_ADDRESS
    .long MULTIBOOT_LOAD_END_ADDRESS
    .long MULTIBOOT_BSS_END_ADDRESS
    .long MULTIBOOT_ENTRY_ADDRESS
    .long MULTIBOOT_MODE_TYPE
    .long MULTIBOOT_FB_WIDTH
    .long MULTIBOOT_FB_HEIGHT
    .long MULTIBOOT_FB_BPP
