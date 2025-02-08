/*
 * shutdown when [messages]
 *
 * allow super users to tell users and remind users
 * of imminent shutdown of unix
 * and shut it down automatically
 * and even reboot or halt the machine if they desire
 *
 * Copyright (c) 1983,1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>
#include <utmp.h>
#include <pwd.h>
#include <paths.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/param.h>
#include <sys/syslog.h>

#define	REBOOT	"/sbin/reboot"
#define	HALT	"/sbin/halt"
#define MAXINTS 20
#define	HOURS	*3600
#define MINUTES	*60
#define SECONDS
#define NLOG		600		/* no of bytes possible for message */
#define	NOLOGTIME	5 MINUTES
#define IGNOREUSER	"sleeper"

char	hostname[MAXHOSTNAMELEN];

struct	utmp utmp;
int	sint;
long	stogo;
char	tpath[] =	"/dev/";
int	nlflag = 1;		/* nolog yet to be done */
int	killflg = 1;
int	doreboot = 0;
int	halt = 0;
int     fast = 0;
char    *nosync = NULL;
char    nosyncflag[] = "-n";
char	term[sizeof tpath + sizeof utmp.ut_line];
char	tbuf[BUFSIZ];
char	nolog1[] = "\n\nNO LOGINS: System going down at %5.5s\n\n";
char	nolog2[NLOG+1];
#ifdef	DEBUG
char	nologin[] = "nologin";
char    fastboot[] = "fastboot";
#else
char	nologin[] = _PATH_NOLOGIN;
char	fastboot[] = "/fastboot";
#endif
time_t	nowtime;
jmp_buf	alarmbuf;

struct interval {
	int stogo;
	int sint;
} interval[] = {
	4 HOURS,	1 HOURS,
	2 HOURS,	30 MINUTES,
	1 HOURS,	15 MINUTES,
	30 MINUTES,	10 MINUTES,
	15 MINUTES,	5 MINUTES,
	10 MINUTES,	5 MINUTES,
	5 MINUTES,	3 MINUTES,
	2 MINUTES,	1 MINUTES,
	1 MINUTES,	30 SECONDS,
	0 SECONDS,	0 SECONDS
};

char *shutter;

static time_t getsdt(char *s);
static void nolog(time_t sdt);
static void doitfast(void);

void finish(sig)
        int sig;
{
	(void) signal(SIGTERM, SIG_IGN);
	(void) unlink(nologin);
	exit(0);
}

void timeout(sig)
        int sig;
{
	longjmp(alarmbuf, 1);
}

void warning(term, sdt, now, type)
	FILE *term;
	time_t sdt, now;
	char *type;
{
	char *ts;
	register int delay = sdt - now;

	if (delay > 8)
		while (delay % 5)
			delay++;

	fprintf(term,
	    "\007\007\t*** %sSystem shutdown message from %s@%s ***\r\n\n",
		    type, shutter, hostname);

	ts = ctime(&sdt);
	if (delay > 10 MINUTES)
		fprintf(term, "System going down at %5.5s\r\n", ts+11);
	else if (delay > 95 SECONDS) {
		fprintf(term, "System going down in %d minute%s\r\n",
		    (delay+30)/60, (delay+30)/60 != 1 ? "s" : "");
	} else if (delay > 0) {
		fprintf(term, "System going down in %d second%s\r\n",
		    delay, delay != 1 ? "s" : "");
	} else
		fprintf(term, "System going down IMMEDIATELY\r\n");
}

int main(argc,argv)
	int argc;
	char **argv;
{
	register int i, ufd;
	register char *f;
	char *ts;
	time_t sdt;
	int h, m;
	int first;
	FILE *termf;
	struct passwd *pw;

	shutter = getlogin();
	if (shutter == 0 && (pw = getpwuid(getuid())))
		shutter = pw->pw_name;
	if (shutter == 0)
		shutter = "???";
	(void) gethostname(hostname, sizeof (hostname));
	openlog("shutdown", 0, LOG_AUTH);
	argc--, argv++;
	while (argc > 0 && (f = argv[0], *f++ == '-')) {
		while ((i = *f++)) switch (i) {
		case 'k':
			killflg = 0;
			continue;
		case 'n':
			nosync = nosyncflag;
			continue;
		case 'f':
			fast = 1;
			continue;
		case 'r':
			doreboot = 1;
			continue;
		case 'h':
			halt = 1;
			continue;
		default:
			fprintf(stderr, "shutdown: '%c' - unknown flag\n", i);
			exit(1);
		}
		argc--, argv++;
	}
	if (argc < 1) {
	        /* argv[0] is not available after the argument handling. */
		printf("Usage: shutdown [ -krhfn ] shutdowntime [ message ]\n");
		finish(0);
	}
	if (fast && (nosync == nosyncflag)) {
	        printf ("shutdown: Incompatible switches 'fast' & 'nosync'\n");
		finish(0);
	}
	if (geteuid()) {
		fprintf(stderr, "NOT super-user\n");
		finish(0);
	}
	nowtime = time((long *)0);
	sdt = getsdt(argv[0]);
	argc--, argv++;
	nolog2[0] = '\0';
	while (argc-- > 0) {
		(void) strcat(nolog2, " ");
		(void) strcat(nolog2, *argv++);
	}
	m = ((stogo = sdt - nowtime) + 30)/60;
	h = m/60;
	m %= 60;
	ts = ctime(&sdt);
	printf("Shutdown at %5.5s (in ", ts+11);
	if (h > 0)
		printf("%d hour%s ", h, h != 1 ? "s" : "");
	printf("%d minute%s) ", m, m != 1 ? "s" : "");
#ifndef DEBUG
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
#endif
	(void) signal(SIGTTOU, SIG_IGN);
	(void) signal(SIGTERM, finish);
	(void) signal(SIGALRM, timeout);
	(void) setpriority(PRIO_PROCESS, 0, PRIO_MIN);
	(void) fflush(stdout);
#ifndef DEBUG
	if ((i = fork())) {
		printf("[pid %d]\n", i);
		exit(0);
	}
#else
	(void) putc('\n', stdout);
#endif
	sint = 1 HOURS;
	f = "";
	ufd = open(_PATH_UTMP,0);
	if (ufd < 0) {
		err(1, "%s", _PATH_UTMP);
		/* NOTREACHED */
	}
	first = 1;
	for (;;) {
		for (i = 0; stogo <= interval[i].stogo && interval[i].sint; i++)
			sint = interval[i].sint;
		if (stogo > 0 && (stogo-sint) < interval[i].stogo)
			sint = stogo - interval[i].stogo;
		if (stogo <= NOLOGTIME && nlflag) {
			nlflag = 0;
			nolog(sdt);
		}
		if (sint >= stogo || sint == 0)
			f = "FINAL ";
		nowtime = time((long *)0);
		(void) lseek(ufd, 0L, 0);
		while (read(ufd,(char *)&utmp,sizeof utmp)==sizeof utmp)
		if (utmp.ut_name[0] &&
		    strncmp(utmp.ut_name, IGNOREUSER, sizeof(utmp.ut_name))) {
			if (setjmp(alarmbuf))
				continue;
			(void) strcpy(term, tpath);
			(void) strncat(term, utmp.ut_line, sizeof utmp.ut_line);
			(void) alarm(3);
#ifdef DEBUG
			if ((termf = stdout) != NULL)
#else
			if ((termf = fopen(term, "w")) != NULL)
#endif
			{
				(void) alarm(0);
				setbuf(termf, tbuf);
				fprintf(termf, "\n\r\n");
				warning(termf, sdt, nowtime, f);
				if (first || sdt - nowtime > 1 MINUTES) {
					if (*nolog2)
						fprintf(termf, "\t...%s", nolog2);
				}
				(void) fputc('\r', termf);
				(void) fputc('\n', termf);
				(void) alarm(5);
#ifdef DEBUG
				(void) fflush(termf);
#else
				(void) fclose(termf);
#endif
				(void) alarm(0);
			}
		}
		if (stogo <= 0) {
			printf("\n\007\007System shutdown time has arrived\007\007\n");
			syslog(LOG_CRIT, "%s by %s: %s",
			    doreboot ? "reboot" : halt ? "halt" : "shutdown",
			    shutter, nolog2);
			sleep(2);
			(void) unlink(nologin);
			if (!killflg) {
				printf("but you'll have to do it yourself\n");
				finish(0);
			}
			if (fast)
				doitfast();
#ifndef DEBUG
			if (doreboot)
				execle(REBOOT, "reboot", "-l", nosync, 0, 0);
			if (halt)
				execle(HALT, "halt", "-l", nosync, 0, 0);
			(void) kill(1, SIGTERM);	/* to single user */
#else
			if (doreboot)
				printf("REBOOT");
			if (halt)
				printf(" HALT");
			if (fast)
				printf(" -l %s (without fsck's)\n", nosync);
			else
				printf(" -l %s\n", nosync);
			else
				printf("kill -HUP 1\n");

#endif
			finish(0);
		}
		stogo = sdt - time((long *) 0);
		if (stogo > 0 && sint > 0)
			sleep((unsigned)(sint<stogo ? sint : stogo));
		stogo -= sint;
		first = 0;
	}
}

time_t
getsdt(s)
	register char *s;
{
	time_t t, t1, tim;
	register char c;
	struct tm *lt;

	if (strcmp(s, "now") == 0)
		return(nowtime);
	if (*s == '+') {
		++s;
		t = 0;
		for (;;) {
			c = *s++;
			if (!isdigit(c))
				break;
			t = t * 10 + c - '0';
		}
		if (t <= 0)
			t = 5;
		t *= 60;
		tim = time((long *) 0) + t;
		return(tim);
	}
	t = 0;
	while (strlen(s) > 2 && isdigit(*s))
		t = t * 10 + *s++ - '0';
	if (*s == ':')
		s++;
	if (t > 23)
		goto badform;
	tim = t*60;
	t = 0;
	while (isdigit(*s))
		t = t * 10 + *s++ - '0';
	if (t > 59)
		goto badform;
	tim += t;
	tim *= 60;
	t1 = time((long *) 0);
	lt = localtime(&t1);
	t = lt->tm_sec + lt->tm_min*60 + (long)lt->tm_hour*3600;
	if (tim < t || tim >= ((long)24*3600)) {
		/* before now or after midnight */
		printf("That must be tomorrow\nCan't you wait till then?\n");
		finish(0);
	}
	return (t1 + tim - t);
badform:
	printf("Bad time format\n");
	finish(0);
	/*NOTREACHED*/
	return 0;
}

void doitfast()
{
	register FILE *fastd;

	if ((fastd = fopen(fastboot, "w")) != NULL) {
		putc('\n', fastd);
		(void) fclose(fastd);
	}
}

void nolog(sdt)
	time_t sdt;
{
	register FILE *nologf;

	(void) unlink(nologin);			/* in case linked to std file */
	if ((nologf = fopen(nologin, "w")) != NULL) {
		fprintf(nologf, nolog1, (ctime(&sdt)) + 11);
		if (*nolog2)
			fprintf(nologf, "\t%s\n", nolog2 + 1);
		(void) fclose(nologf);
	}
}
