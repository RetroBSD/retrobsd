#include "uucp.h"
#ifdef BSD4_2
#include <sys/time.h>
#else
#include <time.h>
#endif
#if defined(USG) || defined(BSD4_2)
#include <fcntl.h>
#endif
#include <sys/stat.h>

static FILE *Lp = NULL;
static FILE *Sp = NULL;
static int Ltried = 0;
static int Stried = 0;

static void mlogent(FILE *fp, char *status, char *text);

/*LINTLIBRARY*/

/*
 *	make log entry
 */
void logent(
char *text, char *status)
{
#ifdef LOGBYSITE
	char lfile[MAXFULLNAME];
	static char LogRmtname[64];
#endif
	if (Rmtname[0] == '\0')
		strcpy(Rmtname, Myname);
	/* Open the log file if necessary */
#ifdef LOGBYSITE
	if (strcmp(Rmtname, LogRmtname)) {
		if (Lp != NULL)
			fclose(Lp);
		Lp = NULL;
		Ltried = 0;
	}
#endif
	if (Lp == NULL) {
		if (!Ltried) {
			int savemask;
#ifdef F_SETFL
			int flags;
#endif
			savemask = umask(LOGMASK);
#ifdef LOGBYSITE
			(void) sprintf(lfile, "%s/%s/%s", LOGBYSITE, Progname, Rmtname);
			strcpy(LogRmtname, Rmtname);
			Lp = fopen (lfile, "a");
#else
			Lp = fopen (LOGFILE, "a");
#endif
			umask(savemask);
#ifdef F_SETFL
			flags = fcntl(fileno(Lp), F_GETFL, 0);
			fcntl(fileno(Lp), F_SETFL, flags|O_APPEND);
#endif
		}
		Ltried = 1;
		if (Lp == NULL)
			return;
		fioclex(fileno(Lp));
	}

	/*  make entry in existing temp log file  */
	mlogent(Lp, status, text);
}

/*
 *	make a log entry
 */
void mlogent(
	register FILE *fp,
	char *status,
	char *text)
{
	static int pid = 0;
	register struct tm *tp;

	if (text == NULL)
		text = "";
	if (status == NULL)
		status = "";
	if (!pid)
		pid = getpid();

	gettimeofday(&Now, NULL);

	tp = localtime(&Now.tv_sec);
#ifdef USG
	fprintf(fp, "%s %s (%d/%d-%2.2d:%2.2d-%d) ",
#else
	fprintf(fp, "%s %s (%d/%d-%02d:%02d-%d) ",
#endif
		User, Rmtname, tp->tm_mon + 1, tp->tm_mday,
		tp->tm_hour, tp->tm_min, pid);
	fprintf(fp, "%s (%s)\n", status, text);

	/* Since it's buffered */
#ifndef F_SETFL
	lseek (fileno(fp), (long)0, 2);
#endif
	fflush (fp);
	if (Debug) {
		fprintf(stderr, "%s %s ", User, Rmtname);
#ifdef USG
		fprintf(stderr, "(%d/%d-%2.2d:%2.2d-%d) ", tp->tm_mon + 1,
			tp->tm_mday, tp->tm_hour, tp->tm_min, pid);
#else
		fprintf(stderr, "(%d/%d-%02d:%02d-%d) ", tp->tm_mon + 1,
			tp->tm_mday, tp->tm_hour, tp->tm_min, pid);
#endif
		fprintf(stderr, "%s (%s)\n", status, text);
	}
}

/*
 *	close log file
 */
void logcls()
{
	if (Lp != NULL)
		fclose(Lp);
	Lp = NULL;
	Ltried = 0;

	if (Sp != NULL)
		fclose (Sp);
	Sp = NULL;
	Stried = 0;
}

/*
 *	make system log entry
 */
void syslog(
char *text)
{
	register struct tm *tp;
#ifdef LOGBYSITE
	char lfile[MAXFULLNAME];
	static char SLogRmtname[64];

	if (strcmp(Rmtname, SLogRmtname)) {
		if (Sp != NULL)
			fclose(Sp);
		Sp = NULL;
		Stried = 0;
	}
#endif
	if (Sp == NULL) {
		if (!Stried) {
			int savemask;
#ifdef F_SETFL
			int flags;
#endif
			savemask = umask(LOGMASK);
#ifdef LOGBYSITE
			(void) sprintf(lfile, "%s/xferstats/%s", LOGBYSITE, Rmtname);
			strcpy(SLogRmtname, Rmtname);
			Sp = fopen (lfile, "a");
#else
			Sp = fopen (SYSLOG, "a");
#endif
			umask(savemask);
#ifdef F_SETFL
			flags = fcntl(fileno(Sp), F_GETFL, 0);
			fcntl(fileno(Sp), F_SETFL, flags|O_APPEND);
#endif

		}
		Stried = 1;
		if (Sp == NULL)
			return;
		fioclex(fileno(Sp));
	}

	gettimeofday(&Now, NULL);

	tp = localtime(&Now.tv_sec);

	fprintf(Sp, "%s %s ", User, Rmtname);
#ifdef USG
	fprintf(Sp, "(%d/%d-%2.2d:%2.2d) ", tp->tm_mon + 1,
		tp->tm_mday, tp->tm_hour, tp->tm_min);
	fprintf(Sp, "(%ld) %s\n", Now.tv_sec, text);
#else
	fprintf(Sp, "(%d/%d-%02d:%02d) ", tp->tm_mon + 1,
		tp->tm_mday, tp->tm_hour, tp->tm_min);
	fprintf(Sp, "(%ld.%02u) %s\n", Now.tv_sec, Now.tv_usec/10000, text);
#endif

	/* Position at end and flush */
#ifndef F_SETFL
	lseek (fileno(Sp), (long)0, 2);
#endif
	fflush (Sp);
}

/*
 * Arrange to close fd on exec(II).
 * Otherwise unwanted file descriptors are inherited
 * by other programs.  And that may be a security hole.
 */
#ifndef	USG
#include <sgtty.h>
#endif

void fioclex(
int fd)
{
	register int ret;

#if defined(USG) || defined(BSD4_2)
	ret = fcntl(fd, F_SETFD, 1);	/* Steve Bellovin says this does it */
#else
	ret = ioctl(fd, FIOCLEX, STBNULL);
#endif
	if (ret) {
		DEBUG(2, "CAN'T FIOCLEX %d\n", fd);
        }
}
