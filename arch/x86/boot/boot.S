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
#include <multiboot2.h>

SECTION(.multiboot, "a", 8)
#define MULTIBOOT2_HEADER_FLAGS (MULTIBOOT2_ARCHITECTURE_I386)
#define MULTIBOOT2_HEADER_SIZE (multiboot_header_end - multiboot_header)
#define MULTIBOOT2_CHECKSUM -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_HEADER_FLAGS + MULTIBOOT2_HEADER_SIZE)

#define MULTIBOOT2_FB_WIDTH  1024
#define MULTIBOOT2_FB_HEIGHT  768
#define MULTIBOOT2_FB_BPP      32

.code32
multiboot_header:
    .long MULTIBOOT2_HEADER_MAGIC
    .long MULTIBOOT2_HEADER_FLAGS
    .long MULTIBOOT2_HEADER_SIZE
    .long MULTIBOOT2_CHECKSUM
framebuffer_tag_start:
        .short MULTIBOOT2_HEADER_TAG_FRAMEBUFFER
        .short MULTIBOOT2_HEADER_TAG_OPTIONAL
        .long framebuffer_tag_end - framebuffer_tag_start
        .long MULTIBOOT2_FB_WIDTH
        .long MULTIBOOT2_FB_HEIGHT
        .long MULTIBOOT2_FB_BPP
framebuffer_tag_end:
        .quad MULTIBOOT2_HEADER_TAG_END
multiboot_header_end:
