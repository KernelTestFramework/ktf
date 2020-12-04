/*
 * pfmlib_intel_ivbep_unc_cbo.c : Intel IvyBridge-EP C-Box uncore PMU
 *
 * Copyright (c) 2014 Google Inc. All rights reserved
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
#include "events/intel_ivbep_unc_cbo_events.h"
#include "pfmlib_intel_snbep_unc_priv.h"
#include "pfmlib_intel_x86_priv.h"
#include "pfmlib_priv.h"

static void display_cbo(void *this, pfmlib_event_desc_t *e, void *val) {
    const intel_x86_entry_t *pe = this_pe(this);
    pfm_snbep_unc_reg_t *reg = val;
    pfm_snbep_unc_reg_t f;

    __pfm_vbprintf("[UNC_CBO=0x%" PRIx64 " event=0x%x umask=0x%x en=%d "
                   "inv=%d edge=%d thres=%d tid_en=%d] %s\n",
                   reg->val, reg->cbo.unc_event, reg->cbo.unc_umask, reg->cbo.unc_en,
                   reg->cbo.unc_inv, reg->cbo.unc_edge, reg->cbo.unc_thres,
                   reg->cbo.unc_tid, pe[e->event].name);

    if (e->count == 1)
        return;

    f.val = e->codes[1];

    __pfm_vbprintf("[UNC_CBOX_FILTER0=0x%" PRIx64 " tid=%d core=0x%x"
                   " state=0x%x]\n",
                   f.val, f.ivbep_cbo_filt0.tid, f.ivbep_cbo_filt0.cid,
                   f.ivbep_cbo_filt0.state);

    if (e->count == 2)
        return;

    f.val = e->codes[2];

    __pfm_vbprintf("[UNC_CBOX_FILTER1=0x%" PRIx64 " nid=%d opc=0x%x"
                   " nc=0x%x isoc=0x%x]\n",
                   f.val, f.ivbep_cbo_filt1.nid, f.ivbep_cbo_filt1.opc,
                   f.ivbep_cbo_filt1.nc, f.ivbep_cbo_filt1.isoc);
}

#define DEFINE_C_BOX(n)                                                                  \
    pfmlib_pmu_t intel_ivbep_unc_cb##n##_support = {                                     \
        .desc = "Intel Ivy Bridge-EP C-Box " #n " uncore",                               \
        .name = "ivbep_unc_cbo" #n,                                                      \
        .perf_name = "uncore_cbox_" #n,                                                  \
        .pmu = PFM_PMU_INTEL_IVBEP_UNC_CB##n,                                            \
        .pme_count = LIBPFM_ARRAY_SIZE(intel_ivbep_unc_c_pe),                            \
        .type = PFM_PMU_TYPE_UNCORE,                                                     \
        .num_cntrs = 4,                                                                  \
        .num_fixed_cntrs = 0,                                                            \
        .max_encoding = 2,                                                               \
        .pe = intel_ivbep_unc_c_pe,                                                      \
        .atdesc = snbep_unc_mods,                                                        \
        .flags = PFMLIB_PMU_FL_RAW_UMASK | INTEL_PMU_FL_UNC_CBO,                         \
        .pmu_detect = pfm_intel_ivbep_unc_detect,                                        \
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
        .can_auto_encode = pfm_intel_x86_can_auto_encode,                                \
        .display_reg = display_cbo,                                                      \
    }

DEFINE_C_BOX(0);
DEFINE_C_BOX(1);
DEFINE_C_BOX(2);
DEFINE_C_BOX(3);
DEFINE_C_BOX(4);
DEFINE_C_BOX(5);
DEFINE_C_BOX(6);
DEFINE_C_BOX(7);
DEFINE_C_BOX(8);
DEFINE_C_BOX(9);
DEFINE_C_BOX(10);
DEFINE_C_BOX(11);
DEFINE_C_BOX(12);
DEFINE_C_BOX(13);
DEFINE_C_BOX(14);
