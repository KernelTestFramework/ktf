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
#include <setup.h>

#include <mm/pmm.h>
#include <mm/vmm.h>

/* Used by higher level mmap_range() functions - must be taken before vmap_lock */
static spinlock_t mmap_lock = SPINLOCK_INIT;

void *get_free_pages(unsigned int order, gfp_flags_t flags) {
    frame_t *frame;
    void *va = NULL;
    mfn_t mfn;

    if (!boot_flags.virt)
        panic("Unable to use %s() before final page tables are set", __func__);

    frame = get_free_frames(order);
    if (!frame)
        return va;
    mfn = frame->mfn;

    spin_lock(&mmap_lock);
    if (flags == GFP_USER) {
        va = vmap_kern(mfn_to_virt_user(mfn), mfn, order, L4_PROT, L3_PROT, L2_PROT,
                       L1_PROT);
        vmap_user(mfn_to_virt_user(mfn), mfn, order, L4_PROT_USER, L3_PROT_USER,
                  L2_PROT_USER, L1_PROT_USER);
    }

    if (flags & GFP_IDENT) {
        va = vmap_kern(mfn_to_virt(mfn), mfn, order, L4_PROT, L3_PROT, L2_PROT, L1_PROT);
        if (flags & GFP_USER)
            vmap_user(mfn_to_virt(mfn), mfn, order, L4_PROT, L3_PROT, L2_PROT, L1_PROT);
    }

    if (flags & GFP_KERNEL) {
        va = kmap(mfn, order, L4_PROT, L3_PROT, L2_PROT, L1_PROT);
        if (flags & GFP_USER)
            vmap_user(mfn_to_virt_kern(mfn), mfn, order, L4_PROT, L3_PROT, L2_PROT,
                      L1_PROT);
    }

    if (flags & GFP_KERNEL_MAP) {
        va = vmap_kern(mfn_to_virt_map(mfn), mfn, order, L4_PROT, L3_PROT, L2_PROT,
                       L1_PROT);
        if (flags & GFP_USER)
            vmap_user(mfn_to_virt_map(mfn), mfn, order, L4_PROT, L3_PROT, L2_PROT,
                      L1_PROT);
    }
    spin_unlock(&mmap_lock);

    return va;
}

void put_pages(void *page) {
    unsigned int order;
    mfn_t mfn;

    spin_lock(&mmap_lock);
    BUG_ON(vunmap_kern(page, &mfn, &order));
    spin_unlock(&mmap_lock);
    put_free_frames(mfn, order);
}