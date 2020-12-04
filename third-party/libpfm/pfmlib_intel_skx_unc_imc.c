/*
 * pfmlib_intel_bdx_unc_imc.c : Intel SkylakeX Integrated Memory Controller (IMC) uncore
 * PMU
 *
 * Copyright (c) 2017 Google LLC
 * Contributed by Stephane Eranian <eranian@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/*
#include <sys/types.h>
*/
/*
#include <ctype.h>
*/
/*
#include <string.h>
*/
/*
#include <stdlib.h>
*/
/*
#include <stdio.h>
*/

#include <ktf.h>
#include <string.h>
/* private headers */
#include "events/intel_skx_unc_imc_events.h"
#include "pfmlib_intel_snbep_unc_priv.h"
#include "pfmlib_intel_x86_priv.h"
#include "pfmlib_priv.h"

#define DEFINE_IMC(n)                                                                    \
    pfmlib_pmu_t intel_skx_unc_imc##n##_support = {                                      \
        .desc = "Intel SkylakeX IMC" #n " uncore",                                       \
        .name = "skx_unc_imc" #n,                                                        \
        .perf_name = "uncore_imc_" #n,                                                   \
        .pmu = PFM_PMU_INTEL_SKX_UNC_IMC##n,                                             \
        .pme_count = LIBPFM_ARRAY_SIZE(intel_skx_unc_m_pe),                              \
        .type = PFM_PMU_TYPE_UNCORE,                                                     \
        .num_cntrs = 4,                                                                  \
        .num_fixed_cntrs = 1,                                                            \
        .max_encoding = 1,                                                               \
        .pe = intel_skx_unc_m_pe,                                                        \
        .atdesc = snbep_unc_mods,                                                        \
        .flags = PFMLIB_PMU_FL_RAW_UMASK,                                                \
        .pmu_detect = pfm_intel_skx_unc_detect,                                          \
        .get_event_encoding[PFM_OS_NONE] = pfm_intel_snbep_unc_get_encoding,             \
        PFMLIB_ENCODE_PERF(pfm_intel_snbep_unc_get_perf_encoding),                       \
        PFMLIB_OS_DETECT(pfm_intel_x86_perf_detect),                                     \
        .get_event_first = pfm_intel_x86_get_event_first,                                \
        .get_event_next = pfm_intel_x86_get_event_next,                                  \
        .event_is_valid = pfm_intel_x86_event_is_valid,                                  \
        .validate_table = pfm_intel_x86_validate_table,                                  \
        .get_event_info = pfm_intel_x86_get_event_info,                                  \
        .get_event_attr_info = pfm_intel_x86_get_event_attr_info,                        \
        PFMLIB_VALID_PERF_PATTRS(pfm_intel_snbep_unc_perf_validate_pattrs),              \
        .get_event_nattrs = pfm_intel_x86_get_event_nattrs,                              \
    };

DEFINE_IMC(0);
DEFINE_IMC(1);
DEFINE_IMC(2);
DEFINE_IMC(3);
DEFINE_IMC(4);
DEFINE_IMC(5);
