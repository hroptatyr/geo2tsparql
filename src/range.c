/*** range.c -- ranges over instants or idiffs
 *
 * Copyright (C) 2013-2016 Sebastian Freundt
 *
 * Author:  Sebastian Freundt <freundt@ga-group.nl>
 *
 * This file is part of geo2tsparql.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the author nor the names of any contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ***/
#if defined HAVE_CONFIG_H
# include "config.h"
#endif	/* HAVE_CONFIG_H */
#include "range.h"


static echs_range_t
_range_unfix(echs_range_t r)
{
	if (r.beg.H == ECHS_ALL_DAY) {
		r.beg.H = 0, r.beg.M = 0, r.beg.S = 0, r.beg.ms = 0;
	} else if (r.beg.ms == ECHS_ALL_SEC) {
		r.beg.ms = 0;
	}
	if (r.end.H == ECHS_ALL_DAY) {
		r.end.H = 0, r.end.M = 0, r.end.S = 0, r.end.ms = 0;
		r.end.d++;
	} else if (r.end.ms == ECHS_ALL_SEC) {
		r.end.S++;
	}
	r.end = echs_instant_fixup(r.end);
	return r;
}

static echs_range_t
_range_fixup(echs_range_t r)
{
	r.beg.H += ECHS_ALL_DAY +
		(echs_min_instant_p(r.beg) || r.beg.H || r.end.H);
	r.end.H += ECHS_ALL_DAY +
		(echs_max_instant_p(r.end) || r.beg.H || r.end.H);

	if (r.end.H == ECHS_ALL_DAY) {
		r.end = echs_instant_add(r.end, (echs_idiff_t){-1});
	}
	return r;
}


echs_idrng_t
echs_range_diff(echs_range_t rng, echs_instant_t rel)
{
/* for ranges we slightly employ different semantics,
 * BEG's all day starts at midnight, while END's all day ends at 24:00 */
	if (rng.beg.H == ECHS_ALL_DAY) {
		rng.beg.H = 0, rng.beg.M = 0, rng.beg.S = 0, rng.beg.ms = 0;
	} else if (rng.beg.ms == ECHS_ALL_SEC) {
		rng.beg.ms = 0;
	}
	return (echs_idrng_t){
		echs_instant_diff(rng.beg, rel),
			echs_instant_diff(rng.end, rel)};
}

echs_range_t
echs_range_add(echs_idrng_t idr, echs_instant_t rel)
{
	return (echs_range_t){
		echs_instant_add(rel, idr.lower),
			echs_instant_add(rel, idr.upper)};
}

echs_range_t
echs_range_coalesce(echs_range_t r1, echs_range_t r2)
{
	echs_range_t r;

	r1 = _range_unfix(r1);
	r2 = _range_unfix(r2);
	if (echs_instant_le_p(r1.end, r2.end) &&
	    echs_instant_le_p(r2.beg, r1.end)) {
		r = (echs_range_t){r1.beg, r2.end};
	} else if (echs_instant_lt_p(r2.end, r1.end) &&
		   echs_instant_le_p(r1.beg, r2.end)) {
		r = (echs_range_t){r2.beg, r1.end};
	} else {
		/* no need fixing up a nul-range is there */
		return echs_nul_range();
	}
	return _range_fixup(r);
}

/* range.c ends here */
