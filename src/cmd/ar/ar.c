/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Hugh Smith at The University of Guelph.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifdef CROSS
#   include <stdint.h>
#   include <stdio.h>
#   include <string.h>
#   include <stdlib.h>
#   include <errno.h>
#   include <getopt.h>
#else
#   include <sys/param.h>
#   include <sys/dir.h>
#   include <stdio.h>
#   include <string.h>
#   include <stdlib.h>
#   include <unistd.h>
#   include <errno.h>
#endif
#include <ar.h>
#include "archive.h"
#include "extern.h"

extern int errno;

CHDR chdr;
unsigned options;
char *archive, *envtmp, *posarg, *posname;

static void
usage()
{
	(void)fprintf(stderr, "Usage:\n");
	(void)fprintf(stderr, "  ar -d [-Tv] archive file ...\n");
	(void)fprintf(stderr, "  ar -m [-Tv] archive file ...\n");
	(void)fprintf(stderr, "  ar -m [-abiTv] position archive file ...\n");
	(void)fprintf(stderr, "  ar -p [-Tv] archive [file ...]\n");
	(void)fprintf(stderr, "  ar -q [-cTv] archive file ...\n");
	(void)fprintf(stderr, "  ar -r [-cuTv] archive file ...\n");
	(void)fprintf(stderr, "  ar -r [-abciuTv] position archive file ...\n");
	(void)fprintf(stderr, "  ar -t [-Tv] archive [file ...]\n");
	(void)fprintf(stderr, "  ar -x [-ouTv] archive [file ...]\n");
	(void)fprintf(stderr, "Commands:\n");
	(void)fprintf(stderr, "  -d      Delete file(s) from the archive\n");
	(void)fprintf(stderr, "  -m      Move file(s) in the archive\n");
	(void)fprintf(stderr, "  -p      Print file(s) found in the archive\n");
	(void)fprintf(stderr, "  -q      Quick append file(s) to the archive\n");
	(void)fprintf(stderr, "  -r      Replace existing or insert new file(s) into the archive\n");
	(void)fprintf(stderr, "  -t      Display contents of archive\n");
	(void)fprintf(stderr, "  -x      Extract file(s) from the archive\n");
	(void)fprintf(stderr, "Modifiers:\n");
	(void)fprintf(stderr, "  -a      Put file(s) after [member-name]\n");
	(void)fprintf(stderr, "  -b, -i  Put file(s) before [member-name]\n");
	(void)fprintf(stderr, "  -c      Do not warn if the library had to be created\n");
	(void)fprintf(stderr, "  -u      Only replace files that are newer than current archive contents\n");
	(void)fprintf(stderr, "  -o      Preserve original dates\n");
	(void)fprintf(stderr, "  -T      Make a thin archive\n");
	(void)fprintf(stderr, "  -v      Be verbose\n");
	exit(1);
}

static void
badoptions(arg)
	char *arg;
{
	(void)fprintf(stderr,
	    "ar: illegal option combination for %s.\n", arg);
	usage();
}

/*
 * main --
 *	main basically uses getopt to parse options and calls the appropriate
 *	functions.  Some hacks that let us be backward compatible with 4.3 ar
 *	option parsing and sanity checking.
 */
int
main(argc, argv)
	int argc;
	char **argv;
{
	extern int optind;
	int c;
	char *p;
	int (*fcall)() = 0;

	if (argc < 3)
		usage();

	/*
	 * Historic versions didn't require a '-' in front of the options.
	 * Fix it, if necessary.
	*/
	if (*argv[1] != '-') {
		if (!(p = malloc((unsigned)(strlen(argv[1]) + 2)))) {
			(void)fprintf(stderr, "ar: %s.\n", strerror(errno));
			exit(1);
		}
		*p = '-';
		(void)strcpy(p + 1, argv[1]);
		argv[1] = p;
	}

	while ((c = getopt(argc, argv, "abcdilmopqrTtuvx")) != EOF) {
		switch(c) {
		case 'a':
			options |= AR_A;
			break;
		case 'b':
		case 'i':
			options |= AR_B;
			break;
		case 'c':
			options |= AR_C;
			break;
		case 'd':
			options |= AR_D;
			fcall = delete;
			break;
		case 'l':		/* not documented, compatibility only */
			envtmp = ".";
			break;
		case 'm':
			options |= AR_M;
			fcall = move;
			break;
		case 'o':
			options |= AR_O;
			break;
		case 'p':
			options |= AR_P;
			fcall = print;
			break;
		case 'q':
			options |= AR_Q;
			fcall = append;
			break;
		case 'r':
			options |= AR_R;
			fcall = replace;
			break;
		case 'T':
			options |= AR_TR;
			break;
		case 't':
			options |= AR_T;
			fcall = contents;
			break;
		case 'u':
			options |= AR_U;
			break;
		case 'v':
			options |= AR_V;
			break;
		case 'x':
			options |= AR_X;
			fcall = extract;
			break;
		default:
			usage();
		}
	}

	argv += optind;
	argc -= optind;

	/* One of -dmpqrtx required. */
	if (!(options & (AR_D|AR_M|AR_P|AR_Q|AR_R|AR_T|AR_X))) {
		(void)fprintf(stderr,
		    "ar: one of options -dmpqrtx is required.\n");
		usage();
	}
	/* Only one of -a and -bi allowed. */
	if (options & AR_A && options & AR_B) {
		(void)fprintf(stderr,
		    "ar: only one of -a and -[bi] options allowed.\n");
		usage();
	}
	/* -ab require a position argument. */
	if (options & (AR_A|AR_B)) {
		if (!(posarg = *argv++)) {
			(void)fprintf(stderr,
			    "ar: no position operand specified.\n");
			usage();
		}
		posname = rname(posarg);
	}
	/* -d only valid with -Tv. */
	if (options & AR_D && options & ~(AR_D|AR_TR|AR_V))
		badoptions("-d");
	/* -m only valid with -abiTv. */
	if (options & AR_M && options & ~(AR_A|AR_B|AR_M|AR_TR|AR_V))
		badoptions("-m");
	/* -p only valid with -Tv. */
	if (options & AR_P && options & ~(AR_P|AR_TR|AR_V))
		badoptions("-p");
	/* -q only valid with -cTv. */
	if (options & AR_Q && options & ~(AR_C|AR_Q|AR_TR|AR_V))
		badoptions("-q");
	/* -r only valid with -abcuTv. */
	if (options & AR_R && options & ~(AR_A|AR_B|AR_C|AR_R|AR_U|AR_TR|AR_V))
		badoptions("-r");
	/* -t only valid with -Tv. */
	if (options & AR_T && options & ~(AR_T|AR_TR|AR_V))
		badoptions("-t");
	/* -x only valid with -ouTv. */
	if (options & AR_X && options & ~(AR_O|AR_U|AR_TR|AR_V|AR_X))
		badoptions("-x");

	if (!(archive = *argv++)) {
		(void)fprintf(stderr, "ar: no archive specified.\n");
		usage();
	}

	/* -dmqr require a list of archive elements. */
	if (options & (AR_D|AR_M|AR_Q|AR_R) && !*argv) {
		(void)fprintf(stderr, "ar: no archive members specified.\n");
		usage();
	}
        if (! fcall)
                exit(1);
	exit((*fcall)(argv));
}
