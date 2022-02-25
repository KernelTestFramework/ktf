/*
 * Copyright (c) 2022 Amazon.com, Inc. or its affiliates.
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
#ifndef KTF_MTRR_H
#define KTF_MTRR_H

#include <ktf.h>

enum mtrr_memory_type {
    MTRR_UC = 0, /* Uncacheable */
    MTRR_WC = 1, /* Write Combining */
    MTRR_WT = 4, /* Write Through */
    MTRR_WP,     /* Write Protected */
    MTRR_WB,     /* Write Back */
};
typedef enum mtrr_memory_type mtrr_memory_type_t;

union mtrr_cap {
    struct {
        /* clang-format off */
        uint64_t vcnt : 8,
		 fix : 1,
		 _rsvd : 1,
		 wc : 1,
		 _rsvd2 : 53;
        /* clang-format on */
    } __packed;
    uint64_t reg;
};
typedef union mtrr_cap mtrr_cap_t;

union mtrr_def_type {
    struct {
        /* clang-format off */
        uint64_t type : 8,
		 _rsvd : 2,
		 fe : 1,
		 e : 1,
		 _rsvd2 : 52;
        /* clang-format on */
    } __packed;
    uint64_t reg;
};
typedef union mtrr_def_type mtrr_def_type_t;

union mtrr_base {
    struct {
        /* clang-format off */
        uint64_t type : 8,
		 _rsvd : 4,
		 phys_base : 40,
		 _rsvd2 : 12;
        /* clang-format on */
    } __packed;
    uint64_t reg;
};
typedef union mtrr_base mtrr_base_t;

union mtrr_mask {
    struct {
        /* clang-format off */
        uint64_t _rsvd : 10,
		 valid : 1,
		 phys_base : 40,
		 _rsvd2 : 13;
        /* clang-format on */
    } __packed;
    uint64_t reg;
};
typedef union mtrr_mask mtrr_mask_t;

extern mtrr_cap_t mtrr_read_cap(void);
extern mtrr_def_type_t mtrr_read_def_type(void);
extern bool mtrr_set_phys_base(mtrr_base_t base, uint8_t reg);
extern bool mtrr_set_phys_mask(mtrr_mask_t mask, uint8_t reg);

#endif /* KTF_MTRR_H */
