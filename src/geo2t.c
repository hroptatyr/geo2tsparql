/*** geo2t.c -- convert geometries to time and vice versa
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

#define MAX_GEOFLT	(90. * 128.)
#define MIN_GEOFLT	(-MAX_GEOFLT)


/* time and geo magic */
static inline __attribute__((const, pure)) echs_idiff_t
geoflt2idiff(double x)
{
	double ipart = trunc(x);
	return ipart <= MIN_GEOFLT
		? echs_min_idiff()
		: ipart >= MAX_GEOFLT
		? echs_max_idiff()
		: (echs_idiff_t){
		(int32_t)ipart,
			(uint32_t)((x - ipart) * (double)MSECS_PER_DAY)};
}

static void
geo2t(double from[static 2U], double to[static 2U])
{
	echs_idrng_t v;
	echs_idrng_t s;
	echs_range_t valid;
	echs_range_t systm;
	char buf[256];
	size_t z = 0U;

	/* scale */
#if 1
	from[0U] = scalbn(from[0U], 7);
	from[1U] = scalbn(from[1U], 7);
	to[0U] = scalbn(to[0U], 7);
	to[1U] = scalbn(to[1U], 7);
#else
	from[0U] *= 100.;
	from[1U] *= 100.;
	to[0U] *= 100.;
	to[1U] *= 100.;
#endif

	v.lower = geoflt2idiff(from[0U]);
	v.upper = geoflt2idiff(to[0U]);
	v.upper.dpart -=
		!echs_max_idiff_p(v.upper) && !v.lower.intra && !v.upper.intra;
	s.lower = geoflt2idiff(from[1U]);
	s.upper = geoflt2idiff(to[1U]);
	valid = echs_range_add(v, reftm);
	valid.beg.H += ECHS_ALL_DAY +
		(echs_min_idiff_p(v.lower) || v.lower.intra || v.upper.intra);
	valid.end.H += ECHS_ALL_DAY +
		(echs_max_idiff_p(v.upper) || v.lower.intra || v.upper.intra);
	systm = echs_range_add(s, reftm);

	z += range_strf(buf + z, sizeof(buf) - z, valid);
	buf[z++] = ',';
	buf[z++] = ' ';
	z += range_strf(buf + z, sizeof(buf) - z, systm);
	buf[z] = '\0';
	fwrite(buf, 1, z, stdout);
	return;
}


static int
geo2t_box2d(const char *box, size_t len)
{
	/* coords */
	double from[2U];
	double to[2U];

	/* read over whitespace */
	for (; len && isspace(*box); box++, len--);
	with (char *eo = NULL) {
		from[0U] = strtod(box, &eo);
		if (UNLIKELY(eo == NULL)) {
			return -1;
		}
		len -= eo - box, box = eo;
	}
	/* more whitespace */
	for (; len && isspace(*box); box++, len--);
	with (char *eo = NULL) {
		from[1U] = strtod(box, &eo);
		if (UNLIKELY(eo == NULL)) {
			return -1;
		}
		len -= eo - box, box = eo;
	}
	/* whitespace */
	for (; len && isspace(*box); box++, len--);
	if (UNLIKELY(*box++ != ',')) {
		/* what sort of 2d data is this? */
		return -1;
	}

	for (; len && isspace(*box); box++, len--);
	with (char *eo = NULL) {
		to[0U] = strtod(box, &eo);
		if (UNLIKELY(eo == NULL)) {
			return -1;
		}
		len -= eo - box, box = eo;
	}
	/* whitespace */
	for (; len && isspace(*box); box++, len--);
	with (char *eo = NULL) {
		to[1U] = strtod(box, &eo);
		if (UNLIKELY(eo == NULL)) {
			return -1;
		}
		len -= eo - box, box = eo;
	}

	geo2t(from, to);
	return 0;
}

static int
geo2t_ln(const char *wkt, size_t len)
{
	static const char box[] = "BOX";
	static const char col[] = "GEOMETRYCOLLECTION";
	size_t ni = 0U;
	size_t wi = 0U;
	int rc = 0;

	/* allow prefixes */
	with (const char *wp = memchr(wkt, '\t', len)) {
		if (wp != NULL) {
			wi += ++wp - wkt;
			fwrite(wkt, 1, wi, stdout);
		}
	}

	if (wi + strlenof(col) < len && !memcmp(wkt + wi, col, strlenof(col))) {
		if (UNLIKELY(wkt[wi += strlenof(col)] != '(')) {
			rc = -1;
			goto out;
		} else if (UNLIKELY(wkt[--len] != '\n')) {
			rc = -1;
			goto out;
		} else if (UNLIKELY(wkt[--len] != ')')) {
			rc = -1;
			goto out;
		}
		/* otherwise it's all good */
		wi++;
	}
	/* read boxes */
	while (wi + strlenof(box) < len) {
		const char *eo;

		/* overread whitespace and commas */
		for (; wi < len &&
			     (isspace(wkt[wi]) || wkt[wi] == ','); wi++, ni++);

		if (memcmp(wkt + wi, box, strlenof(box))) {
			/* nope, not a box */
			break;
		}
		/* optionally allow 2D */
		if (wkt[wi += strlenof(box)] == '2') {
			if (UNLIKELY(wkt[++wi] != 'D')) {
				rc = -1;
				break;
			}
			wi++;
		}
		if (UNLIKELY(wkt[wi++] != '(')) {
			rc = -1;
			break;
		}
		/* find matching paren */
		eo = memchr(wkt + wi, ')', len - wi);
		if (UNLIKELY(eo == NULL)) {
			/* it's not even a matching pair of parens */
			rc = -1;
			break;
		}
		/* geospatial data */
		if (ni) {
			fputc(';', stdout);
			fputc(' ', stdout);
		}
		rc += geo2t_box2d(wkt + wi, eo - (wkt + wi));
		/* advance wi */
		wi = ++eo - wkt;
	}
out:
	/* in which case we finalise the line */
	fputc('\n', stdout);
	return rc;
}


#include "geo2t.yucc"

int
main(int argc, char *argv[])
{
	yuck_t argi[1U];
	int rc = 0;

	if (yuck_parse(argi, argc, argv) < 0) {
		rc = 1;
		goto out;
	}

	if (!argi->nargs) {
		char *line = NULL;
		size_t llen = 0U;

		for (ssize_t nrd; (nrd = getline(&line, &llen, stdin)) > 0;) {
			rc |= geo2t_ln(line, nrd) < 0;
		}
	}

out:
	yuck_free(argi);
	return rc;
}

/* geo2t.c ends here */
