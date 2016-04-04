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
#include "dt-strpf.h"
#include "nifty.h"


/* time and geo magic */
static void
geo2t(double from[static 2U], double to[static 2U])
{
	/* scale */
	from[0U] *= 1000.;
	from[1U] *= 1000.;
	to[0U] *= 1000.;
	to[1U] *= 1000.;

	with (double fromd = floor(from[0U])) {
		printf("%f %f\n", fromd, from[0U] - fromd);
	}
	return;
}

static void
t2geo(echs_range_t valid, echs_range_t systm)
{
	return;
}


static int
geo2t_box2d(const char *box, size_t len)
{
	/* it's either dates or geo coords */
	if (memchr(box, 'Z', len)) {
		/* dates */
		echs_range_t valid;
		echs_range_t systm;

		/* read over whitespace */
		for (; len && isspace(*box); box++, len--);
		with (char *eo = NULL) {
			valid = range_strp(box, &eo, len);
			if (UNLIKELY(eo == NULL)) {
				return -1;
			}
			len -= eo - box, box = eo;
		}

		/* should be sep'd by comma */
		if (UNLIKELY(*box++ != ',')) {
			/* what sort of 2d data is this? */
			return -1;
		}

		/* more whitespace */
		for (; len && isspace(*box); box++, len--);
		with (char *eo = NULL) {
			systm = range_strp(box, &eo, len);
			if (UNLIKELY(eo == NULL)) {
				return -1;
			}
			len -= eo - box, box = eo;
		}

		t2geo(valid, systm);
	} else {
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
	}
	return 0;
}

static int
geo2t_ln(const char *wkt, size_t len)
{
	static const char box[] = "BOX";
	size_t wi;

	/* overread whitespace */
	for (wi = 0U; wi < len && isspace(wkt[wi]); wi++);

	if (UNLIKELY(wi + strlenof(box) >= len)) {
		return -1;
	} else if (UNLIKELY(memcmp(wkt + wi, box, strlenof(box)))) {
		return -1;
	}
	/* parse the box data */
	switch (wkt[wi += strlenof(box)]) {
	case '2':
		if (UNLIKELY(wkt[++wi] != 'D')) {
			return -1;
		}
		wi++;
		/*@fallthrough@*/
	case '(':
		wi++;
		break;
	case '3':
		/* 3D times?  Efficacy?  No, not on my watch! */
	default:
		return -1;
	}
	with (const char *eo = memchr(wkt + wi, ')', len - wi)) {
		if (UNLIKELY(eo == NULL)) {
			/* it's not even a matching pair of parens */
			return -1;
		}
		/* just use that box parser now */
		return geo2t_box2d(wkt + wi, eo - (wkt + wi));
	}
	/* shouldn't be reached */
	return 0;
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
