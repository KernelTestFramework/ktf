/*
 * Copyright © 2022 Amazon.com, Inc. or its affiliates.
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

#include <mtrr.h>
#include <pagetable.h>

mtrr_cap_t mtrr_read_cap(void) {
    mtrr_cap_t c;
    c.reg = rdmsr(MSR_MTRR_CAP);
    return c;
}

mtrr_def_type_t mtrr_read_def_type(void) {
    mtrr_def_type_t t;
    t.reg = rdmsr(MSR_MTRR_DEF_TYPE);
    return t;
}

bool mtrr_set_phys_base(mtrr_base_t base, uint8_t reg) {
    mtrr_cap_t cap = mtrr_read_cap();
    if (reg < cap.vcnt) {
        wrmsr(MSR_MTRR_PHYS_BASE(reg), base.reg);
        return true;
    }
    return false;
}

bool mtrr_set_phys_mask(mtrr_mask_t mask, uint8_t reg) {
    mtrr_cap_t cap = mtrr_read_cap();
    if (reg < cap.vcnt) {
        wrmsr(MSR_MTRR_PHYS_MASK(reg), mask.reg);
        return true;
    }
    return false;
}
