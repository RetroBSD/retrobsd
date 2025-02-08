/*
 * Copyright (c) 1989, 1993
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
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#define PF(f, func) { \
	if (fieldwidth) \
		if (precision) \
			(void)printf(f, fieldwidth, precision, func); \
		else \
			(void)printf(f, fieldwidth, func); \
	else if (precision) \
		(void)printf(f, precision, func); \
	else \
		(void)printf(f, func); \
}

static char **gargv;

static int	 asciicode();
static void	 escape();
static int	 getchr();
static int	 getint();
static int	 getlong();
static char	*getstr();
static char	*mklong();
static void	 usage();
#ifndef NOFPU
static double	 getdouble();
#endif

int
#ifdef BUILTIN
progprintf(argc, argv)
#else
main(argc, argv)
#endif
	int argc;
	char *argv[];
{
	extern int optind;
	static char *skip1, *skip2;
	int ch, end, fieldwidth, precision;
	char convch, nextch, *format, *start;
	register char *fmt;

	while ((ch = getopt(argc, argv, "")) != EOF)
		switch (ch) {
		case '?':
		default:
			usage();
			return (1);
		}
	argc -= optind;
	argv += optind;

	if (argc < 1) {
		usage();
		return (1);
	}

	/*
	 * Basic algorithm is to scan the format string for conversion
	 * specifications -- once one is found, find out if the field
	 * width or precision is a '*'; if it is, gather up value.  Note,
	 * format strings are reused as necessary to use up the provided
	 * arguments, arguments of zero/null string are provided to use
	 * up the format string.
	 */
	skip1 = "#-+ 0";
	skip2 = "*0123456789";

	escape(fmt = format = *argv);		/* backslash interpretation */
	gargv = ++argv;
	for (;;) {
		end = 0;
		/* find next format specification */
next:		for (start = fmt;; ++fmt) {
			if (!*fmt) {
				/* avoid infinite loop */
				if (end == 1) {
					warnx("missing format character",
					    NULL, NULL);
					return (1);
				}
				end = 1;
				if (fmt > start)
					(void)printf("%s", start);
				if (!*gargv)
					return (0);
				fmt = format;
				goto next;
			}
			/* %% prints a % */
			if (*fmt == '%') {
				if (*++fmt != '%')
					break;
				*fmt++ = '\0';
				(void)printf("%s", start);
				goto next;
			}
		}

		/* skip to field width */
		for (; strchr(skip1, *fmt); ++fmt);
		if (*fmt == '*') {
			if (getint(&fieldwidth))
				return (1);
		} else
			fieldwidth = 0;

		/* skip to possible '.', get following precision */
		for (; strchr(skip2, *fmt); ++fmt);
		if (*fmt == '.')
			++fmt;
		if (*fmt == '*') {
			if (getint(&precision))
				return (1);
		} else
			precision = 0;

		/* skip to conversion char */
		for (; strchr(skip2, *fmt); ++fmt);
		if (!*fmt) {
			warnx("missing format character", NULL, NULL);
			return (1);
		}

		convch = *fmt;
		nextch = *++fmt;
		*fmt = '\0';
		switch(convch) {
		case 'c': {
			char p;

			p = getchr();
			PF(start, p);
			break;
		}
		case 's': {
			char *p;

			p = getstr();
			PF(start, p);
			break;
		}
		case 'd': case 'i': case 'o': case 'u': case 'x': case 'X': {
			long p;
			char *f;

			if ((f = mklong(start, convch)) == NULL)
				return (1);
			if (getlong(&p))
				return (1);
			PF(f, p);
			break;
		}
#ifndef NOFPU
		case 'e': case 'E': case 'f': case 'g': case 'G': {
			double p;

			p = getdouble();
			PF(start, p);
			break;
		}
#endif
		default:
			warnx("illegal format character", NULL, NULL);
			return (1);
		}
		*fmt = nextch;
	}
	/* NOTREACHED */
}

static char *
mklong(str, ch)
	register char *str;
	int ch;
{
	static char copy[64];
	register int len;

	if (ch == 'X')		/* XXX */
		ch = 'x';
	len = strlen(str) + 2;
	bcopy(str, copy, len - 3);
	copy[len - 3] = 'l';
	copy[len - 2] = ch;
	copy[len - 1] = '\0';
	return (copy);
}

static void
escape(fmt)
	register char *fmt;
{
	register char *store;
	register int value;
	int c;

	for (store = fmt; (c = *fmt); ++fmt, ++store) {
		if (c != '\\') {
			*store = c;
			continue;
		}
		switch (*++fmt) {
		case '\0':		/* EOS, user error */
			*store = '\\';
			*++store = '\0';
			return;
		case '\\':		/* backslash */
		case '\'':		/* single quote */
			*store = *fmt;
			break;
		case 'a':		/* bell/alert */
			*store = '\7';
			break;
		case 'b':		/* backspace */
			*store = '\b';
			break;
		case 'f':		/* form-feed */
			*store = '\f';
			break;
		case 'n':		/* newline */
			*store = '\n';
			break;
		case 'r':		/* carriage-return */
			*store = '\r';
			break;
		case 't':		/* horizontal tab */
			*store = '\t';
			break;
		case 'v':		/* vertical tab */
			*store = '\13';
			break;
					/* octal constant */
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
			for (c = 3, value = 0;
			    c-- && *fmt >= '0' && *fmt <= '7'; ++fmt) {
				value <<= 3;
				value += *fmt - '0';
			}
			--fmt;
			*store = value;
			break;
		default:
			*store = *fmt;
			break;
		}
	}
	*store = '\0';
}

static int
getchr()
{
	if (!*gargv)
		return ('\0');
	return ((int)**gargv++);
}

static char *
getstr()
{
	if (!*gargv)
		return ("");
	return (*gargv++);
}

static char *Number = "+-.0123456789";
static int
getint(ip)
	int *ip;
{
	long val;

	if (getlong(&val))
		return (1);
	if (val > 65535) {
		warnx("%s: %s", *gargv, strerror(ERANGE));
		return (1);
	}
	*ip = val;
	return (0);
}

static int
getlong(lp)
	register long *lp;
{
	long val;
	char *ep;

	if (!*gargv) {
		*lp = 0;
		return (0);
	}
	if (strchr(Number, **gargv)) {
		errno = 0;
		val = strtol(*gargv, &ep, 0);
		if (*ep != '\0') {
			warnx("%s: illegal number", *gargv, NULL);
			return (1);
		}
		if (errno == ERANGE)
			if (val == 2147483647L) {
				warnx("%s: %s", *gargv, strerror(ERANGE));
				return (1);
			}
			if (val == (-2147483647L-1)) {
				warnx("%s: %s", *gargv, strerror(ERANGE));
				return (1);
			}

		*lp = val;
		++gargv;
		return (0);
	}
	*lp =  (long)asciicode();
	return (0);
}

#ifndef NOFPU
static double
getdouble()
{
	if (!*gargv)
		return ((double)0);
	if (strchr(Number, **gargv))
		return (atof(*gargv++));
	return ((double)asciicode());
}
#endif

static int
asciicode()
{
	register int ch;

	ch = **gargv;
	if (ch == '\'' || ch == '"')
		ch = (*gargv)[1];
	++gargv;
	return (ch);
}

static void
usage()
{
	(void)fprintf(stderr, "usage: printf format [arg ...]\n");
}
