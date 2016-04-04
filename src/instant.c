/*** instant.c -- some echs_instant_t functionality
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

#include "instant.h"
#include "nifty.h"

static const unsigned int doy[] = {
	0U, 0U, 31U, 59U, 90U, 120U, 151U, 181U, 212U, 243U, 273U, 304U, 334U,
	365U, 396U, 424U, 455U, 485U, 516U, 546U, 577U, 608U, 638U, 669U, 699U,
};

static __attribute__((const, pure)) inline unsigned int
__get_ndom(unsigned int y, unsigned int m)
{
/* return the number of days in month M in year Y. */
	static const unsigned int mdays[] = {
		0U, 31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U,
	};
	unsigned int res = mdays[m];

	if (UNLIKELY(!(y % 4U) && m == 2U)) {
		res++;
	}
	return res;
}

static inline unsigned int
__doy(echs_instant_t i)
{
	unsigned int res = doy[i.m] + i.d;

	if (UNLIKELY((i.y % 4U) == 0) && i.m >= 3) {
		res++;
	}
	return res;
}


echs_instant_t
echs_instant_fixup(echs_instant_t e)
{
/* this is basically __ymd_fixup_d of dateutils
 * we only care about additive cockups though because instants are
 * chronologically ascending */
	unsigned int md;

	if (UNLIKELY(echs_instant_all_day_p(e))) {
		/* just fix up the day, dom and year portion */
		goto fixup_d;
	} else if (UNLIKELY(echs_instant_all_sec_p(e))) {
		/* just fix up the sec, min, ... portions */
		goto fixup_S;
	}

	if (UNLIKELY(e.ms >= MSECS_PER_SEC)) {
		unsigned int dS = e.ms / MSECS_PER_SEC;
		unsigned int ms = e.ms % MSECS_PER_SEC;

		e.ms = ms;
		e.S += dS;
	}

fixup_S:
	if (UNLIKELY(e.S >= SECS_PER_MIN)) {
		/* leap seconds? */
		unsigned int dM = e.S / SECS_PER_MIN;
		unsigned int S = e.S % SECS_PER_MIN;

		e.S = S;
		e.M += dM;
	}
	if (UNLIKELY(e.M >= MINS_PER_HOUR)) {
		unsigned int dH = e.M / MINS_PER_HOUR;
		unsigned int M = e.M % MINS_PER_HOUR;

		e.M = M;
		e.H += dH;
	}
	if (UNLIKELY(e.H >= HOURS_PER_DAY)) {
		unsigned int dd = e.H / HOURS_PER_DAY;
		unsigned int H = e.H % HOURS_PER_DAY;

		e.H = H;
		e.d += dd;
	}

fixup_d:
refix_ym:
	if (UNLIKELY(e.m > 12U)) {
		unsigned int dy = (e.m - 1) / 12U;
		unsigned int m = (e.m - 1) % 12U + 1U;

		e.m = m;
		e.y += dy;
	}

	if (UNLIKELY(e.d > (md = __get_ndom(e.y, e.m)))) {
		e.d -= md;
		e.m++;
		goto refix_ym;
	}
	return e;
}

echs_idiff_t
echs_instant_diff(echs_instant_t end, echs_instant_t beg)
{
	int extra_df;
	int intra_df;

	if (UNLIKELY(echs_max_instant_p(end) || echs_min_instant_p(beg))) {
		return echs_max_idiff();
	}

	if (end.H == ECHS_ALL_DAY) {
		end.H = 24U, end.M = 0, end.S = 0, end.ms = 0;
	} else if (end.ms == ECHS_ALL_SEC) {
		end.ms = 1000;
	}
	if (beg.H == ECHS_ALL_DAY) {
		beg.H = 0, beg.M = 0, beg.S = 0, beg.ms = 0;
	} else if (beg.ms == ECHS_ALL_SEC) {
		beg.ms = 0;
	}

	/* just see what the intraday part yields for the difference */
	intra_df = end.H - beg.H;
	intra_df *= MINS_PER_HOUR;
	intra_df += end.M - beg.M;
	intra_df *= SECS_PER_MIN;
	intra_df += end.S - beg.S;
	intra_df *= MSECS_PER_SEC;
	intra_df += end.ms - beg.ms;

	if (intra_df < 0) {
		intra_df += MSECS_PER_DAY;
		extra_df = -1;
	} else if (LIKELY((unsigned int)intra_df < MSECS_PER_DAY)) {
		extra_df = 0;
	} else {
		intra_df -= MSECS_PER_DAY;
		extra_df = 1;
	}

	{
		unsigned int dom_end = __doy(end);
		unsigned int dom_beg = __doy(beg);
		int df_y = end.y - beg.y;

		extra_df += dom_end - dom_beg;
		if (echs_instant_lt_p(beg, end) && extra_df < 0) {
			df_y--;
		}
		extra_df += df_y * (int)DAYS_PER_YEAR + (df_y - 1) / 4;
	}

	return (echs_idiff_t){extra_df, intra_df};
}

echs_instant_t
echs_instant_add(echs_instant_t bas, echs_idiff_t add)
{
	echs_instant_t res = bas;
	int dd = add.dpart;
	int msd = add.intra;
	int car, cdr;

	if (UNLIKELY(echs_max_instant_p(bas))) {
		return echs_max_instant();
	} else if (UNLIKELY(echs_max_idiff_p(add))) {
		return echs_max_instant();
	} else if (UNLIKELY(echs_min_instant_p(bas))) {
		return echs_min_instant();
	}

	if (UNLIKELY(echs_instant_all_day_p(bas))) {
		/* just fix up the day, dom and year portion */
		goto fixup_d;
	} else if (UNLIKELY(echs_instant_all_sec_p(bas))) {
		/* just fix up the sec, min, ... portions */
		msd /= (int)MSECS_PER_SEC;
		goto fixup_S;
	}

	car = (res.ms + msd) / (int)MSECS_PER_SEC;
	if ((cdr = (res.ms + msd) % (int)MSECS_PER_SEC) >= 0) {
		res.ms = cdr;
	} else {
		res.ms = cdr + MSECS_PER_SEC;
		car--;
	}
	msd = car;
fixup_S:
	car = (res.S + msd) / (int)SECS_PER_MIN;
	if ((cdr = (res.S + msd) % (int)SECS_PER_MIN) >= 0) {
		res.S = cdr;
	} else {
		res.S = cdr + SECS_PER_MIN;
		car--;
	}
	msd = car;

	car = ((int)res.M + msd) / (int)MINS_PER_HOUR;
	if ((cdr = ((int)res.M + msd) % (int)MINS_PER_HOUR) >= 0) {
		res.M = cdr;
	} else {
		res.M = cdr + MINS_PER_HOUR;
		car--;
	}
	msd = car;

	car = (res.H + msd) / (int)HOURS_PER_DAY;
	if ((cdr = (res.H + msd) % (int)HOURS_PER_DAY) >= 0) {
		res.H = cdr;
	} else {
		res.H = cdr + HOURS_PER_DAY;
		car--;
	}
	msd = car;

	/* get ready to adjust the day */
	if (UNLIKELY(msd)) {
		dd += msd;
	}
	if (dd) {
		int df_y;
		int df_m;
		int y;
		int m;
		int d;

	fixup_d:
		y = bas.y + dd / (int)DAYS_PER_YEAR;
		if ((df_y = y - bas.y)) {
			dd -= df_y * (int)DAYS_PER_YEAR + (df_y - 1) / 4;
		}

		m = bas.m + dd / 31;
		if ((df_m = m - bas.m)) {
			dd -= doy[bas.m + df_m] - doy[bas.m + 1];
		}

		d = bas.d + dd;
		while (d <= 0) {
			while (--m <= 0) {
				y--;
				m = 12U;
			}
			d += __get_ndom(y, m);
		}
		while ((unsigned int)d > __get_ndom(y, m)) {
			d -= __get_ndom(y, m);
			while (++m > 12) {
				y++;
				m = 1U;
			}
		}

		res.d = d;
		res.m = m;
		res.y = y;
	}
	return res;
}

/* instant.c ends here */
