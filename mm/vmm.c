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
#include <pagetable.h>
#include <setup.h>

#include <mm/pmm.h>
#include <mm/vmm.h>

/* Used by higher level mmap_range() functions - must be taken before vmap_lock */
static spinlock_t mmap_lock = SPINLOCK_INIT;

static inline vmap_flags_t gfp_to_vmap_flags(gfp_flags_t gfp_flags) {
    vmap_flags_t vmap_flags = VMAP_NONE;

    if (gfp_flags == GFP_USER)
        return VMAP_KERNEL_USER | VMAP_USER;

    if (gfp_flags & GFP_IDENT) {
        vmap_flags |= VMAP_IDENT;
        if (gfp_flags & GFP_USER)
            vmap_flags |= VMAP_USER_IDENT;
    }

    if (gfp_flags & GFP_KERNEL) {
        vmap_flags |= VMAP_KERNEL;
        if (gfp_flags & GFP_USER)
            vmap_flags |= VMAP_USER_KERNEL;
    }

    if (gfp_flags & GFP_KERNEL_MAP) {
        vmap_flags |= VMAP_KERNEL_MAP;
        if (gfp_flags & GFP_USER)
            vmap_flags |= VMAP_USER_KERNEL_MAP;
    }

    return vmap_flags;
}

static inline void *gfp_mfn_to_virt(gfp_flags_t gfp_flags, mfn_t mfn) {
    /* Return virtual address if a single area is specified ... */
    switch (gfp_flags) {
    case GFP_IDENT:
        return mfn_to_virt(mfn);
    case GFP_KERNEL_MAP:
        return mfn_to_virt_map(mfn);
    case GFP_USER:
        return mfn_to_virt_user(mfn);
    case GFP_KERNEL:
        return mfn_to_virt_kern(mfn);
    default:
        /* Otherwise, return kernel addresses if specified before identity
         * mapping or user. The below order reflects most common uses.
         */
        if (gfp_flags & GFP_KERNEL_MAP)
            return mfn_to_virt_map(mfn);
        else if (gfp_flags & GFP_KERNEL)
            return mfn_to_virt_kern(mfn);
        else if (gfp_flags & GFP_IDENT)
            return mfn_to_virt(mfn);
        else if (gfp_flags & GFP_USER)
            return mfn_to_virt_user(mfn);
    }

    return NULL;
}

void *get_free_pages(unsigned int order, gfp_flags_t gfp_flags) {
    void *va = NULL;
    frame_t *frame;
    mfn_t mfn;
    size_t size;
    unsigned long pt_flags;
    vmap_flags_t vmap_flags;

    ASSERT(gfp_flags != GFP_NONE);

    if (!boot_flags.virt)
        panic("Unable to use %s() before final page tables are set", __func__);

    frame = get_free_frames(order);
    if (!frame)
        return va;
    mfn = frame->mfn;

    size = ORDER_TO_SIZE(order);
    pt_flags = order_to_flags(order);
    vmap_flags = gfp_to_vmap_flags(gfp_flags);

    spin_lock(&mmap_lock);
    if (vmap_range(mfn_to_paddr(mfn), size, pt_flags, vmap_flags) == 0)
        va = gfp_mfn_to_virt(gfp_flags, mfn);
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