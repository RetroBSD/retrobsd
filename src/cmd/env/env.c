/*
 * Copyright (c) 1988, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

static void
usage(void)
{
	(void)fprintf(stderr, "usage: env [-i] [name=value ...] [command]\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	char **ep, *p;
	char *cleanenv[1];
	int ch;

	while ((ch = getopt(argc, argv, "-i")) != -1)
		switch((char)ch) {
		case '-':			/* obsolete */
		case 'i':
			environ = cleanenv;
			cleanenv[0] = NULL;
			break;
		case '?':
		default:
			usage();
		}

	for (argv += optind; *argv && (p = strchr(*argv, '=')); ++argv)
		(void)setenv(*argv, ++p, 1);

	if (*argv) {
		/* return 127 if the command to be run could not be found; 126
		   if the command was was found but could not be invoked */

		execvp(*argv, argv);
		err((errno == ENOENT) ? 127 : 126, "%s", *argv);
		/* NOTREACHED */
	}

	for (ep = environ; *ep; ep++)
		(void)printf("%s\n", *ep);

	exit(0);
}
