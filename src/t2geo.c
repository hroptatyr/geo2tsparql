/*** t2geo.c -- convert time to WKT geometries
 *
 * Copyright (C) 2014-2016 Sebastian Freundt
 *
 * Author:  Sebastian Freundt <freundt@ga-group.nl>
 *
 * This file is part of geo2tsparql
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include "dt-strpf.h"
#include "nifty.h"

static const echs_instant_t reftm = {
	.y = 2000,
	.m = 1,
	.d = 1,
	.H = 0,
	.M = 0,
	.S = 0,
	.ms = 0,
};
static const time_t reftm_time = 946684800/*2000-01-01Z*/;
static time_t now;

#define MAX_GEOFLT	(90. * 128.)
#define MIN_GEOFLT	(-MAX_GEOFLT)


/* time and geo magic */
static inline __attribute__((const, pure)) double
idiff2geoflt(echs_idiff_t x)
{
	double r = (double)x.dpart + (double)x.intra / (double)MSECS_PER_DAY;
	return r >= MAX_GEOFLT
		? MAX_GEOFLT
		: r <= MIN_GEOFLT
		? MIN_GEOFLT
		: r;
}

static echs_idrng_t
current_idrng(void)
{
	/* second precision ought to be good enough */
	echs_idiff_t low = {
		.dpart = now / 86400,
		.intra = (now % 86400) * MSECS_PER_SEC,
	};
	return (echs_idrng_t){low, echs_max_idiff()};
}

static void
t2geo(echs_range_t valid, echs_range_t systm)
{
	echs_idrng_t v = echs_range_diff(valid, reftm);
	echs_idrng_t s = echs_nul_instant_p(systm.beg)
		? current_idrng() : echs_range_diff(systm, reftm);
	double from[2U], to[2U];

	from[0U] = idiff2geoflt(v.lower);
	to[0U] = idiff2geoflt(v.upper);
	from[1U] = idiff2geoflt(s.lower);
	to[1U] = !echs_max_idiff_p(s.upper)
		? idiff2geoflt(s.upper) : MAX_GEOFLT;

#if 1
	from[0U] = scalbn(from[0U], -7);
	from[1U] = scalbn(from[1U], -7);
	to[0U] = scalbn(to[0U], -7);
	to[1U] = scalbn(to[1U], -7);
#else
	from[0U] /= 100.;
	from[1U] /= 100.;
	to[0U] /= 100.;
	to[1U] /= 100.;
#endif

	fprintf(stdout, "BOX(%.17f %.17f, %.17f %.17f)",
		from[0U], from[1U], to[0U], to[1U]);
	return;
}


static int
t2geo_ln(const char *wkt, size_t len)
{
	echs_range_t val;
	echs_range_t sys;
	size_t wi = 0U;
	size_t coll = 0U;
	int rc = 0;

	/* allow prefixes */
	with (const char *wp = memchr(wkt, '\t', len)) {
		if (wp != NULL) {
			wi += ++wp - wkt;
			fwrite(wkt, 1, wi, stdout);
		}
	}

	/* read them intervals */
	while (wi < len) {
		char *eo;

		for (; wi < len && isspace(wkt[wi]); wi++);
		val = range_strp(wkt + wi, &eo, len - wi);
		if (UNLIKELY(eo == NULL)) {
			break;
		}
		/* reset WI */
		wi = eo - wkt;
		/* should be sep'd by comma */
		if (wi >= len || wkt[wi++] != ',') {
			sys = echs_max_range();
			if (wi < len) {
				coll++;
			}
		} else {
			/* more whitespace */
			for (; wi < len && isspace(wkt[wi]); wi++);
			if (wi >= len) {
				break;
			}
			sys = range_strp(wkt + wi, &eo, len);
			if (UNLIKELY(eo == NULL)) {
				rc = -1;
				goto oh;
			}
			/* reset WI */
			wi = eo - wkt;
			if (wi < len && wkt[wi] == ';') {
				/* yep, semicolons allowed */
				wi++;
				coll++;
			}
		}
		/* oki then */
		if (coll == 1U) {
			fputs("GEOMETRYCOLLECTION(", stdout);
			coll++;
		} else if (coll > 1U) {
			fputc(',', stdout);
			fputc(' ', stdout);
		}
		/* convert to geospatial */
		t2geo(val, sys);
	}

oh:
	/* finalise the line */
	if (coll) {
		fputc(')', stdout);
	}
	fputc('\n', stdout);
	return rc;
}


#include "t2geo.yucc"

int
main(int argc, char *argv[])
{
	yuck_t argi[1U];
	int rc = 0;

	if (yuck_parse(argi, argc, argv) < 0) {
		rc = 1;
		goto out;
	}

	/* set current time */
	now = time(NULL) - reftm_time;

	if (!argi->nargs) {
		char *line = NULL;
		size_t llen = 0U;

		for (ssize_t nrd; (nrd = getline(&line, &llen, stdin)) > 0;) {
			if (LIKELY(line[nrd - 1U] == '\n')) {
				nrd--;
			}
			rc |= t2geo_ln(line, nrd) < 0;
		}
	}

out:
	yuck_free(argi);
	return rc;
}

/* t2geo.c ends here */
