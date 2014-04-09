/*
 *	execlp(name, arg,...,0)	(like execl, but does path search)
 *	execvp(name, argv)	(like execv, but does path search)
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <paths.h>

static	char shell[] =	"/bin/sh";

int
execlp(name, argv)
	const char *name, *argv;
{
	return execvp (name, (char * const*) &argv);
}

static char *
execat(s1, s2, si)
	register char *s1, *s2;
	char *si;
{
	register char *s;

	s = si;
	while (*s1 && *s1 != ':')
		*s++ = *s1++;
	if (si != s)
		*s++ = '/';
	while (*s2)
		*s++ = *s2++;
	*s = '\0';
	return(*s1? ++s1: 0);
}

int
execvp(name, argv)
	const char *name;
	char *const *argv;
{
	char *pathstr;
	register char *cp;
	char fname[128];
	char *newargs[256];
	int i;
	register unsigned etxtbsy = 1;
	register int eacces = 0;

	pathstr = getenv("PATH");
	if (! pathstr)
		pathstr = _PATH_STDPATH;
	cp = strchr(name, '/') ? "" : pathstr;

	do {
		cp = execat(cp, name, fname);
	retry:
		execv(fname, argv);
		switch(errno) {
		case ENOEXEC:
			newargs[0] = "sh";
			newargs[1] = fname;
			for (i=1; (newargs[i+1] = argv[i]); i++) {
				if (i>=254) {
					errno = E2BIG;
					return(-1);
				}
			}
			execv(shell, newargs);
			return(-1);
		case ETXTBSY:
			if (++etxtbsy > 5)
				return(-1);
			sleep(etxtbsy);
			goto retry;
		case EACCES:
			eacces++;
			break;
		case ENOMEM:
		case E2BIG:
			return(-1);
		}
	} while (cp);
	if (eacces)
		errno = EACCES;
	return(-1);
}
