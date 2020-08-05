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
