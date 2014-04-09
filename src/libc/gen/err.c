/*-
 * Copyright (c) 1993
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

/*
 * Helper routines.  Repeated constructs of the form "%s: " used up too
 * much D space.  On a pdp-11 code can be overlaid but Data space is worth
 * conserving.  An extra function call or two handling an error condition is
 * a reasonable trade for 20 or 30 bytes of D space.
 */
static void
putcolsp()
{
	fputc (':', stderr);
	fputc (' ', stderr);
}

static void
putprog()
{
	fputs (__progname, stderr);
	putcolsp();
}

void
verr (eval, fmt, ap)
	int eval;
	const char *fmt;
	va_list ap;
{
	int sverrno;

	sverrno = errno;
	putprog();
	if (fmt != NULL) {
		(void)vfprintf(stderr, fmt, ap);
		putcolsp();
	}
	(void)fputs(strerror(sverrno), stderr);
	(void)fputc('\n', stderr);
	exit(eval);
}

void
#ifdef __STDC__
err (int eval, const char *fmt, ...)
#else
err (eval, fmt, va_alist)
	int eval;
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;
#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	verr(eval, fmt, ap);
	va_end(ap);
}

void
verrx (eval, fmt, ap)
	int eval;
	const char *fmt;
	va_list ap;
{
	putprog();
	if (fmt != NULL)
		(void)vfprintf(stderr, fmt, ap);
	(void)fputc('\n', stderr);
	exit(eval);
}

void
#if __STDC__
errx (int eval, const char *fmt, ...)
#else
errx (eval, fmt, va_alist)
	int eval;
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;
#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	verrx(eval, fmt, ap);
	va_end(ap);
}

void
vwarn (fmt, ap)
	const char *fmt;
	va_list ap;
{
	int sverrno;

	sverrno = errno;
	putprog();
	if (fmt != NULL) {
		(void)vfprintf(stderr, fmt, ap);
		putcolsp();
	}
	(void)fputs(strerror(sverrno), stderr);
	(void)fputc('\n', stderr);
}

void
#if __STDC__
warn (const char *fmt, ...)
#else
warn(fmt, va_alist)
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;
#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	vwarn(fmt, ap);
	va_end(ap);
}

void
vwarnx (fmt, ap)
	const char *fmt;
	va_list ap;
{
	putprog();
	if (fmt != NULL)
		(void)vfprintf(stderr, fmt, ap);
	(void)fputc('\n', stderr);
}

void
#ifdef __STDC__
warnx (const char *fmt, ...)
#else
warnx(fmt, va_alist)
	char *fmt;
	va_dcl
#endif
{
	va_list ap;
#ifdef __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	vwarnx(fmt, ap);
	va_end(ap);
}
