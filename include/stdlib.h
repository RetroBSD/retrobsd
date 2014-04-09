/*-
 * Copyright (c) 1990, 1993
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
 *
 *	@(#)stdlib.h	8.3.2 (2.11BSD) 1996/1/12
 *
 * Adapted from the 4.4-Lite CD.  The odds of a ANSI C compiler for 2.11BSD
 * being slipped under the door are not distinguishable from 0 - so the
 * prototypes and ANSI ifdefs have been removed from this file.
 *
 * Some functions (strtoul for example) do not exist yet but are retained in
 * this file because additions to libc.a are anticipated shortly.
 */

#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <sys/types.h>

#ifndef NULL
#define	NULL	0
#endif

#define	EXIT_FAILURE	1
#define	EXIT_SUCCESS	0

#define	RAND_MAX	0x7fff

void	abort();
int	abs (int);
int	atexit (void (*)(void));
double	atof();
int	atoi();
long	atol();
void	*calloc (size_t, size_t);
void	exit (int);
void	free (void *);
char	*getenv();
long    labs (long);
void	*malloc (size_t);
char	*mktemp (char *);
int     mkstemp (char *);
void	qsort();
int	rand();
void	*realloc (void*, size_t);
void	srand();
double	strtod();
long	strtol();
unsigned long strtoul();
int	system();

int     putenv (char *string);
int     setenv (const char *name, const char *value, int overwrite);
int     unsetenv (const char *name);
char	*_findenv (const char *name, int *offset);

void	*alloca();

int	daemon();
char	*devname();
int	getloadavg(unsigned loadavg[], int nelem);

extern char *suboptarg;			/* getsubopt(3) external variable */
int	getsubopt();

long	random (void);
char	*setstate (char *);
void	srandom (unsigned);

char *ecvt (double, int, int *, int *);
char *fcvt (double, int, int *, int *);
char *gcvt (double, int, char *);

#endif /* _STDLIB_H_ */
