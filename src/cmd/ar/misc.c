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
#   include <sys/types.h>
#   include <sys/select.h>
#   include <sys/time.h>
#   include <stdio.h>
#   include <string.h>
#   include <stdlib.h>
#   include <stdint.h>
#   include <unistd.h>
#   include <errno.h>
#else
#   include <sys/param.h>
#   include <sys/dir.h>
#   include <stdio.h>
#   include <string.h>
#   include <stdlib.h>
#   include <unistd.h>
#   include <errno.h>
#   include <signal.h>
#   include <paths.h>
#endif
#include "archive.h"
#include "extern.h"

extern CHDR chdr;			/* converted header */
extern char *archive;			/* archive name */
char *tname = "temporary file";		/* temporary file "name" */

int
tmp()
{
	extern char *envtmp;
#ifndef CROSS
	sigset_t set, oset;
#endif
	static int first;
	static const char *artmp = _PATH_ARTMP;
	int fd;
	char path[MAXPATHLEN];

	if (!first && !envtmp) {
		envtmp = (char *)getenv("TMPDIR");
		first = 1;
	}

	if (envtmp) {
		strcpy(path, envtmp);
		strcat(path, strrchr (artmp, '/'));
	} else {
		strcpy(path, artmp);
        }
#ifndef CROSS
	sigfillset(&set);
	(void)sigprocmask(SIG_BLOCK, &set,  &oset);
#endif
	fd = mkstemp(path);
	if (fd == -1)
		error(tname);
        (void)unlink(path);
#ifndef CROSS
	(void)sigprocmask(SIG_SETMASK, &oset, NULL);
#endif
	return(fd);
}

/*
 * files --
 *	See if the current file matches any file in the argument list; if it
 * 	does, remove it from the argument list.
 */
char *
files(argv)
	char **argv;
{
	register char **list;
	char *p;

	for (list = argv; *list; ++list)
		if (compare(*list)) {
			p = *list;
			while ((list[0] = list[1]) != 0)
                            list++;
			return(p);
		}
	return(NULL);
}

void
orphans(argv)
	char **argv;
{
	for (; *argv; ++argv)
		(void)fprintf(stderr,
		    "ar: %s: not found in archive.\n", *argv);
}

char *
rname(path)
	char *path;
{
	register char *ind;

	ind = strrchr(path, '/');
	return(ind ? ind + 1 : path);
}

int
compare(dest)
	char *dest;
{
	if (options & AR_TR)
		return(!strncmp(chdr.name, rname(dest), OLDARMAXNAME));
	return(!strcmp(chdr.name, rname(dest)));
}

void
badfmt()
{
	errno = EINVAL;
	error(archive);
}

void
error(name)
	char *name;
{
	(void)fprintf(stderr, "ar: %s: %s\n", name, strerror(errno));
	exit(1);
}
