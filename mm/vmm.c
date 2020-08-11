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
#include <ktf.h>
#include <lib.h>
#include <page.h>

#include <mm/pmm.h>
#include <mm/vmm.h>

void *get_free_pages(unsigned int order, uint32_t flags) {
    mfn_t mfn = get_free_frames(order);
    void *va = NULL;

    if (mfn_invalid(mfn))
        return NULL;

    if (flags & GFP_IDENT)
        va = vmap(mfn_to_virt(mfn), mfn, order, L1_PROT);
    if (flags & GFP_USER)
        va = vmap(mfn_to_virt_user(mfn), mfn, order, L1_PROT_USER);
    if (flags & GFP_KERNEL)
        va = kmap(mfn, order, L1_PROT);

    return va;
}

void put_pages(void *page, unsigned int order) {
    /* FIXME: unmap all mappings */
    vunmap(page, order);
    put_frame(virt_to_mfn(page), order);
}
