/*
 * pfmlib_intel_hswep_unc_ubo.c : Intel Haswell-EP U-Box uncore PMU
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
#include "pfmlib_priv.h"
#include "pfmlib_intel_x86_priv.h"
#include "pfmlib_intel_snbep_unc_priv.h"
#include "events/intel_hswep_unc_ubo_events.h"

static void
display_ubo(void *this, pfmlib_event_desc_t *e, void *val)
{
	const intel_x86_entry_t *pe = this_pe(this);
	pfm_snbep_unc_reg_t *reg = val;

	__pfm_vbprintf("[UNC_UBO=0x%"PRIx64" event=0x%x umask=0x%x en=%d "
		       "inv=%d edge=%d thres=%d] %s\n",
			reg->val,
			reg->com.unc_event,
			reg->com.unc_umask,
			reg->com.unc_en,
			reg->com.unc_inv,
			reg->com.unc_edge,
			reg->com.unc_thres,
			pe[e->event].name);
}


pfmlib_pmu_t intel_hswep_unc_ubo_support = {
	.desc			= "Intel Haswell-EP U-Box uncore",
	.name			= "hswep_unc_ubo",
	.perf_name		= "uncore_ubox",
	.pmu			= PFM_PMU_INTEL_HSWEP_UNC_UBOX,
	.pme_count		= LIBPFM_ARRAY_SIZE(intel_hswep_unc_u_pe),
	.type			= PFM_PMU_TYPE_UNCORE,
	.num_cntrs		= 2,
	.num_fixed_cntrs	= 1,
	.max_encoding		= 1,
	.pe			= intel_hswep_unc_u_pe,
	.atdesc			= snbep_unc_mods,
	.flags			= PFMLIB_PMU_FL_RAW_UMASK,
	.pmu_detect		= pfm_intel_hswep_unc_detect,
	.get_event_encoding[PFM_OS_NONE] = pfm_intel_snbep_unc_get_encoding,
	 PFMLIB_ENCODE_PERF(pfm_intel_snbep_unc_get_perf_encoding),
	 PFMLIB_OS_DETECT(pfm_intel_x86_perf_detect),
	.get_event_first	= pfm_intel_x86_get_event_first,
	.get_event_next		= pfm_intel_x86_get_event_next,
	.event_is_valid		= pfm_intel_x86_event_is_valid,
	.validate_table		= pfm_intel_x86_validate_table,
	.get_event_info		= pfm_intel_x86_get_event_info,
	.get_event_attr_info	= pfm_intel_x86_get_event_attr_info,
	PFMLIB_VALID_PERF_PATTRS(pfm_intel_snbep_unc_perf_validate_pattrs),
	.get_event_nattrs	= pfm_intel_x86_get_event_nattrs,
	.display_reg		= display_ubo,
};
