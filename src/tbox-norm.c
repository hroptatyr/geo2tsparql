/*** tbox-norm.c -- connect adjacent intervals
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


static int
norm_ln(const char *wkt, size_t len)
{
	echs_range_t last;
	size_t zpre = 0U;
	size_t wi = 0U;
	int rc = 0;

	/* allow prefixes */
	with (const char *wp = memchr(wkt, '\t', len)) {
		if (wp != NULL) {
			wi += ++wp - wkt;
			fwrite(wkt, 1, wi, stdout);
		}
	}

	/* read first interval to set up state */
	with (char *eo = NULL) {
		last = range_strp(wkt + wi, &eo, len - wi);
		if (UNLIKELY(eo == NULL)) {
			rc = -1;
			goto oh;
		}
		/* unfix him */
		last = echs_range_unfix(last);
		/* also fast forward */
		wi = eo - wkt;
		for (; wi < len && isspace(wkt[wi]); wi++);
	}

	while (wi < len) {
		char *eo = NULL;
		echs_range_t this;
		echs_range_t coal;

		this = range_strp(wkt + wi, &eo, len - wi);
		if (UNLIKELY(eo == NULL)) {
			break;
		}
		this = echs_range_unfix(this);

		coal = echs_range_coalesce(last, this);
		if (echs_nul_range_p(coal)) {
			char buf[256U];
			size_t z = zpre;

			buf[0U] = ' ';
			last = echs_range_fixup(last);
			z += range_strf(buf + z, sizeof(buf) - z, last);
			fwrite(buf, 1, z, stdout);
			last = this;
			zpre |= 1U;
		} else {
			last = coal;
		}
		/* fast forward wi */
		wi = eo - wkt;
		for (; wi < len && isspace(wkt[wi]); wi++);
	}

	/* print what we've got */
	if (!echs_nul_range_p(last)) {
		char buf[256U];
		size_t z = zpre;

		buf[0U] = ' ';
		last = echs_range_fixup(last);
		z += range_strf(buf + z, sizeof(buf) - z, last);
		fwrite(buf, 1, z, stdout);
	}

oh:
	/* finalise the line */
	if (wi < len) {
		fwrite(wkt + wi, 1, len - wi, stdout);
	} else {
		fputc('\n', stdout);
	}
	return rc;
}


#include "tbox-norm.yucc"

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
			rc |= norm_ln(line, nrd) < 0;
		}
	}

out:
	yuck_free(argi);
	return rc;
}

/* tbox-norm.c ends here */
