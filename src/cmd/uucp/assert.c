#include "uucp.h"
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>

/*LINTLIBRARY*/

/*
 *	print out assetion error
 */
void assert(char *s1, char *s2, int i1)
{
	register FILE *errlog;
	register struct tm *tp;
	time_t clock;
	int pid;

	errlog = NULL;
	if (!Debug) {
		int savemask;
		savemask = umask(LOGMASK);
		errlog = fopen(ERRLOG, "a");
		umask(savemask);
	}
	if (errlog == NULL)
		errlog = stderr;

	pid = getpid();
	fprintf(errlog, "ASSERT ERROR (%.9s)  ", Progname);
	fprintf(errlog, "pid: %d  ", pid);
	clock = time(NULL);
	tp = localtime(&clock);
#ifdef USG
	fprintf(errlog, "(%d/%d-%2.2d:%2.2d) ", tp->tm_mon + 1,
		tp->tm_mday, tp->tm_hour, tp->tm_min);
#endif
#ifndef USG
	fprintf(errlog, "(%d/%d-%02d:%02d) ", tp->tm_mon + 1,
		tp->tm_mday, tp->tm_hour, tp->tm_min);
#endif
	fprintf(errlog, "%s %s (%d)\n", s1 ? s1 : "", s2 ? s2 : "", i1);
	if (errlog != stderr)
		(void) fclose(errlog);
	return;
}
