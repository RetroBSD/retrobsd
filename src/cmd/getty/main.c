/*
 * getty -- adapt to terminal speed on dialup, and call login
 *
 * Melbourne getty, June 83, kre.
 *
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sgtty.h>
#include <signal.h>
#include <ctype.h>
#include <setjmp.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include "gettytab.h"

extern	char **environ;

struct	sgttyb tmode = {
	0, 0, CERASE, CKILL, 0
};
struct	tchars tc = {
	CINTR, CQUIT, CSTART,
	CSTOP, CEOF, CBRK,
};
struct	ltchars ltc = {
	CSUSP, CDSUSP, CRPRNT,
	CFLUSH, CWERASE, CLNEXT
};

int	crmod;
int	upper;
int	lower;
int	digit;

char	hostname[32];
char	name[16];
char	dev[] = "/dev/";
char	ctty[] = "/dev/console";
char	ttyn[32];

#define	OBUFSIZ		128
#define	TABBUFSIZ	512

char	defent[TABBUFSIZ];
char	defstrs[TABBUFSIZ];
char	tabent[TABBUFSIZ];
char	tabstrs[TABBUFSIZ];

char	*env[128];

char partab[] = {
	0001,0201,0201,0001,0201,0001,0001,0201,
	0202,0004,0003,0205,0005,0206,0201,0001,
	0201,0001,0001,0201,0001,0201,0201,0001,
	0001,0201,0201,0001,0201,0001,0001,0201,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0201
};

#define	ERASE	tmode.sg_erase
#define	KILL	tmode.sg_kill
#define	EOT	tc.t_eofc

static void putpad(char *s);
static void putf(char *cp);
static int getname(void);
static void oflush(void);
static void putchr(int cc);
static void prompt(void);

jmp_buf timeout;

void dingdong(sig)
        int sig;
{
	alarm(0);
	signal(SIGALRM, SIG_DFL);
	longjmp(timeout, 1);
}

jmp_buf	intrupt;

void interrupt(sig)
        int sig;
{
	signal(SIGINT, interrupt);
	longjmp(intrupt, 1);
}

int main(argc, argv)
	char *argv[];
{
	register char *tname;
	long allflags;
	int repcnt = 0;
	int someflags;

	signal(SIGINT, SIG_IGN);
/*
	signal(SIGQUIT, SIG_DFL);
*/
	openlog("getty", LOG_ODELAY|LOG_CONS, LOG_AUTH);
	gethostname(hostname, sizeof(hostname));
	if (hostname[0] == '\0')
		strcpy(hostname, "Amnesiac");
	/*
	 * The following is a work around for vhangup interactions
	 * which cause great problems getting window systems started.
	 * If the tty line is "-", we do the old style getty presuming
	 * that the file descriptors are already set up for us.
	 * J. Gettys - MIT Project Athena.
	 */
	if (argc <= 2 || strcmp(argv[2], "-") == 0)
	    strcpy(ttyn, ttyname(0));
	else {
	    strcpy(ttyn, dev);
	    strncat(ttyn, argv[2], sizeof(ttyn)-sizeof(dev));
	    if (strcmp(argv[0], "+") != 0) {
		chown(ttyn, 0, 0);
		chmod(ttyn, 0622);
		/*
		 * Delay the open so DTR stays down long enough to be detected.
		 */
		//sleep(2);
		while (open(ttyn, O_RDWR) != 0) {
			if (repcnt % 10 == 0) {
				syslog(LOG_ERR, "%s: %m", ttyn);
				closelog();
			}
			repcnt++;
			sleep(60);
		}
		signal(SIGHUP, SIG_IGN);
		vhangup();
		(void) open(ttyn, O_RDWR);
		close(0);
		dup(1);
		dup(0);
		signal(SIGHUP, SIG_DFL);
	    }
	}

	gettable("default", defent, defstrs);
	gendefaults();
	tname = "default";
	if (argc > 1)
		tname = argv[1];
	ioctl(0, TIOCGETP, &tmode);
	for (;;) {
		int ldisp = NTTYDISC;

		gettable(tname, tabent, tabstrs);
		if (OPset || EPset || APset)
			APset++, OPset++, EPset++;
		setdefaults();
		ioctl(0, TIOCFLUSH, 0);		/* clear out the crap */
		if (IS)
			tmode.sg_ispeed = speed(IS);
		else if (SP)
			tmode.sg_ispeed = speed(SP);
		if (OS)
			tmode.sg_ospeed = speed(OS);
		else if (SP)
			tmode.sg_ospeed = speed(SP);
		tmode.sg_flags = setflags(0);
		ioctl(0, TIOCSETP, &tmode);
		setchars();
		ioctl(0, TIOCSETC, &tc);
		ioctl(0, TIOCSETD, &ldisp);
		if (HC)
			ioctl(0, TIOCHPCL, 0);
		if (AB) {
			extern char *autobaud();

			tname = autobaud();
			continue;
		}
		if (PS) {
			tname = portselector();
			continue;
		}
		if (CL && *CL)
			putpad(CL);
		edithost(HE);
		if (IM && *IM)
			putf(IM);
		if (setjmp(timeout)) {
			tmode.sg_ispeed = tmode.sg_ospeed = 0;
			ioctl(0, TIOCSETP, &tmode);
			exit(1);
		}
		if (TO) {
			signal(SIGALRM, dingdong);
			alarm((int)TO);
		}
		if (getname()) {
			register int i;

			oflush();
			alarm(0);
			signal(SIGALRM, SIG_DFL);
			if (!(upper || lower || digit))
				continue;
			allflags = setflags(2);
			tmode.sg_flags = allflags & 0xffff;
			someflags = allflags >> 16;
			if (crmod || NL)
				tmode.sg_flags |= CRMOD;
			ioctl(0, TIOCSETP, &tmode);
			ioctl(0, TIOCSLTC, &ltc);
			ioctl(0, TIOCLSET, &someflags);
			signal(SIGINT, SIG_DFL);
			for (i = 0; environ[i] != (char *)0; i++)
				env[i] = environ[i];
			makeenv(&env[i]);
			execle(LO, "login", "-p", name, (char *) 0, env);
			exit(1);
		}
		alarm(0);
		signal(SIGALRM, SIG_DFL);
		signal(SIGINT, SIG_IGN);
		if (NX && *NX)
			tname = NX;
	}
}

void putstr(s)
	register const char *s;
{
	while (*s)
		putchr(*s++);
}

int getname()
{
	register char *np;
	register int c;
	char cs;

	/*
	 * Interrupt may happen if we use CBREAK mode
	 */
	if (setjmp(intrupt)) {
		signal(SIGINT, SIG_IGN);
		return (0);
	}
	signal(SIGINT, interrupt);
	tmode.sg_flags = setflags(0);
	ioctl(0, TIOCSETP, &tmode);
	tmode.sg_flags = setflags(1);
	prompt();
	if (PF > 0) {
		oflush();
		sleep((int)PF);
		PF = 0;
	}
	ioctl(0, TIOCSETP, &tmode);
	crmod = 0;
	upper = 0;
	lower = 0;
	digit = 0;
	np = name;
	for (;;) {
		oflush();
		if (read(0, &cs, 1) <= 0)
			exit(0);
		if ((c = cs&0177) == 0)
			return (0);
		if (c == EOT)
			exit(1);
		if (c == '\r' || c == '\n' || np >= &name[sizeof name]) {
			putf("\r\n");
			break;
		}
		if (islower(c))
			lower++;
		else if (isupper(c))
			upper++;
		else if (c == ERASE || c == '#' || c == '\b') {
			if (np > name) {
				np--;
				if (tmode.sg_ospeed >= B1200)
					putstr("\b \b");
				else
					putchr(cs);
			}
			continue;
		} else if (c == KILL || c == '@') {
			putchr(cs);
			putchr('\r');
			if (tmode.sg_ospeed < B1200)
				putchr('\n');
			/* this is the way they do it down under ... */
			else if (np > name)
				putstr("                                     \r");
			prompt();
			np = name;
			continue;
		} else if (isdigit(c))
			digit++;
		if (IG && (c <= ' ' || c > 0176))
			continue;
		*np++ = c;
		putchr(cs);
	}
	signal(SIGINT, SIG_IGN);
	*np = 0;
	if (c == '\r')
		crmod++;
	return (1);
}

static
short	tmspc10[] = {
	0, 2000, 1333, 909, 743, 666, 500, 333, 166, 83, 55, 41, 20, 10, 5, 15
};

void putpad(s)
	register char *s;
{
	register int pad = 0;
	register int mspc10;

	if (isdigit(*s)) {
		while (isdigit(*s)) {
			pad *= 10;
			pad += *s++ - '0';
		}
		pad *= 10;
		if (*s == '.' && isdigit(s[1])) {
			pad += s[1] - '0';
			s += 2;
		}
	}
	putstr(s);

	/*
	 * If no delay needed, or output speed is
	 * not comprehensible, then don't try to delay.
	 */
	if (pad == 0)
		return;
	if (tmode.sg_ospeed <= 0 ||
	    tmode.sg_ospeed >= (sizeof tmspc10 / sizeof tmspc10[0]))
		return;

	/*
	 * Round up by a half a character frame,
	 * and then do the delay.
	 * Too bad there are no user program accessible programmed delays.
	 * Transmitting pad characters slows many
	 * terminals down and also loads the system.
	 */
	mspc10 = tmspc10[tmode.sg_ospeed];
	pad += mspc10 / 2;
	for (pad /= mspc10; pad > 0; pad--)
		putchr(*PC);
}

char	outbuf[OBUFSIZ];
int	obufcnt = 0;

void putchr(cc)
{
	char c;

	c = cc;
	if (EP || OP)
                c |= partab[c&0177] & 0200;
	if (OP)
		c ^= 0200;
	if (!UB) {
		outbuf[obufcnt++] = c;
		if (obufcnt >= OBUFSIZ)
			oflush();
	} else
		write(1, &c, 1);
}

void oflush()
{
	if (obufcnt)
		write(1, outbuf, obufcnt);
	obufcnt = 0;
}

void prompt()
{
	putf(LM);
	if (CO)
		putchr('\n');
}

void putf(cp)
	register char *cp;
{
	char *ttyn, *slash;
	char datebuffer[60];
	extern char editedhost[];
	extern char *ttyname(), *rindex();

	while (*cp) {
		if (*cp != '%') {
			putchr(*cp++);
			continue;
		}
		switch (*++cp) {

		case 't':
			ttyn = ttyname(0);
			slash = rindex(ttyn, '/');
			if (slash == (char *) 0)
				putstr(ttyn);
			else
				putstr(&slash[1]);
			break;

		case 'h':
			putstr(editedhost);
			break;

		case 'd':
			get_date(datebuffer);
			putstr(datebuffer);
			break;

		case '%':
			putchr('%');
			break;
		}
		cp++;
	}
}
