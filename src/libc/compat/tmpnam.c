/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <sys/param.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <paths.h>

FILE *
tmpfile()
{
	FILE *fp;
	char *f, *tmpnam();

	if (!(f = tmpnam((char *)NULL)) || !(fp = fopen(f, "w+"))) {
		fprintf(stderr, "tmpfile: cannot open %s.\n", f);
		return(NULL);
	}
	(void)unlink(f);
	return(fp);
}

char *
tmpnam(s)
	char *s;
{
	if (!s && !(s = malloc((u_int)MAXPATHLEN)))
		return(NULL);
	strcpy(s, _PATH_USRTMP "XXXXXX");
	return mktemp(s);
}

char *
tempnam(dir, pfx)
	char *dir, *pfx;
{
	char *f, *name;

	if (!(name = malloc((u_int)MAXPATHLEN)))
		return(NULL);

        f = getenv("TMPDIR");
	if (f) {
		(void)sprintf(name, "%s/%sXXXXXX", f, pfx ? "" : pfx);
		f = mktemp(name);
		if (f)
			return(f);
	}
	if (dir) {
		(void)sprintf(name, "%s/%sXXXXXX", dir, pfx ? "" : pfx);
		f = mktemp(name);
		if (f)
			return(f);
	}
	(void)sprintf(name, _PATH_USRTMP "%sXXXXXX", pfx ? "" : pfx);
	f = mktemp(name);
	if (f)
		return(f);
	(void)sprintf(name, "/tmp/%sXXXXXX", pfx ? "" : pfx);
	return(mktemp(name));
}
