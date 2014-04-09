/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * set teletype modes
 */
#include <stdio.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const struct {
	char	*string;
	int	bval;
	unsigned baud;
} speeds[] = {
	"0",        B0,         0,
	"50",       B50,        50,
	"75",       B75,        75,
	"150",      B150,       150,
	"200",      B200,       200,
	"300",      B300,       300,
	"600",      B600,       600,
	"1200",     B1200,      1200,
	"1800",     B1800,      1800,
	"2400",     B2400,      2400,
	"4800",     B4800,      4800,
	"9600",     B9600,      9600,
	"19200",    B19200,     19200,
	"38400",    B38400,     38400,
	"57600",    B57600,     57600,
	"115200",   B115200,    115200,
        "230400",   B230400,    230400,
        "460800",   B460800,    460800,
        "500000",   B500000,    500000,
        "576000",   B576000,    576000,
        "921600",   B921600,    921600,
        "1000000",  B1000000,   1000000,
        "1152000",  B1152000,   1152000,
        "1500000",  B1500000,   1500000,
        "2000000",  B2000000,   2000000,
        "2500000",  B2500000,   2500000,
        "3000000",  B3000000,   3000000,
        "3500000",  B3500000,   3500000,
        "4000000",  B4000000,   4000000,
	0,
};

struct	MODES {
	char	*string;
	int	set;
	int	reset;
};

struct	MODES	modes[] = {
	"even",		EVENP, 0,
	"-even",	0, EVENP,
	"odd",		ODDP, 0,
	"-odd",		0, ODDP,
	"raw",		RAW, 0,
	"-raw",		0, RAW,
	"cooked",	0, RAW,
	"-nl",		CRMOD, 0,
	"nl",		0, CRMOD,
	"echo",		ECHO, 0,
	"-echo",	0, ECHO,
	"-tabs",	XTABS, 0,
	"tabs",		0, XTABS,
	"tandem",	TANDEM, 0,
	"-tandem",	0, TANDEM,
	"cbreak",	CBREAK, 0,
	"-cbreak",	0, CBREAK,
	0
};

struct MODES lmodes[] = {
	"rtscts",	LRTSCTS, 0,
	"-rtscts",	0, LRTSCTS,
	"crtbs",	LCRTBS, LPRTERA,
	"-crtbs",	0, LCRTBS,
	"prterase",	LPRTERA, LCRTBS+LCRTKIL+LCRTERA,
	"-prterase",	0, LPRTERA,
	"crterase",	LCRTERA, LPRTERA,
	"-crterase",	0, LCRTERA,
	"crtkill",	LCRTKIL, LPRTERA,
	"-crtkill",	0, LCRTKIL,
	"mdmbuf",	LMDMBUF, 0,
	"-mdmbuf",	0, LMDMBUF,
	"litout",	LLITOUT, 0,
	"-litout",	0, LLITOUT,
	"pass8",	LPASS8, 0,
	"-pass8",	0, LPASS8,
	"tostop",	LTOSTOP, 0,
	"-tostop",	0, LTOSTOP,
	"flusho",	LFLUSHO, 0,
	"-flusho",	0, LFLUSHO,
	"nohang",	LNOHANG, 0,
	"-nohang",	0, LNOHANG,
	"ctlecho",	LCTLECH, 0,
	"-ctlecho",	0, LCTLECH,
	"pendin",	LPENDIN, 0,
	"-pendin",	0, LPENDIN,
	"decctlq",	LDECCTQ, 0,
	"-decctlq",	0, LDECCTQ,
	"noflsh",	LNOFLSH, 0,
	"-noflsh",	0, LNOFLSH,
	0
};

struct	MODES mmodes[] = {
	"dcd",		TIOCM_CD, 0,
	"-dcd",		0, TIOCM_CD,
	"dsr",		TIOCM_DSR, 0,
	"-dsr",		0, TIOCM_DSR,
	"dtr",		TIOCM_DTR, 0,
	"-dtr",		0, TIOCM_DTR,
	"cts",		TIOCM_CTS, 0,
	"-cts",		0, TIOCM_CTS,
	"rts",		TIOCM_RTS, 0,
	"-rts",		0, TIOCM_RTS,
	0,
};

struct tchars tc;
struct ltchars ltc;
struct sgttyb mode;
struct winsize win;
int	lmode;
int	ldisc;
int	mstate, nmstate;

struct	special {
	char	*name;
	char	*cp;
	char	def;
} special[] = {
	"erase",	&mode.sg_erase,		CERASE,
	"kill",		&mode.sg_kill,		CKILL,
	"intr",		&tc.t_intrc,		CINTR,
	"quit",		&tc.t_quitc,		CQUIT,
	"start",	&tc.t_startc,		CSTART,
	"stop",		&tc.t_stopc,		CSTOP,
	"eof",		&tc.t_eofc,		CEOF,
	"brk",		&tc.t_brkc,		CBRK,
	"susp",		&ltc.t_suspc,		CSUSP,
	"dsusp",	&ltc.t_dsuspc,		CDSUSP,
	"rprnt",	&ltc.t_rprntc,		CRPRNT,
	"flush",	&ltc.t_flushc,		CFLUSH,
	"werase",	&ltc.t_werasc,		CWERASE,
	"lnext",	&ltc.t_lnextc,		CLNEXT,
	0
};

char	*arg;
char	*Nfmt = "%-8s";

main(argc, argv)
	int	argc;
	register char	**argv;
{
	int i, fmt = 0, ch, zero = 0;
	register struct special *sp;
	char	obuf[BUFSIZ];
	char	*arg2;

	setbuf(stderr, obuf);

	opterr = 0;
	while (optind < argc && strspn(argv[optind], "-aef") == strlen(argv[optind]) &&
		   (ch = getopt(argc, argv, "aef:")) != EOF)
        {
		switch (ch) {
                case 'e':
                case 'a':
                        fmt = 2;
                        break;
                case 'f':
                        i = open(optarg, O_RDONLY|O_NONBLOCK);
                        if (i < 0)
                                err(1, "%s", optarg);
                        if (dup2(i, 1) < 0)
                                err(1, "dup2(%d,1)", i);
                        break;
                case '?':
                default:
                        goto args;
                }
        }
args:
	argc -= optind;
	argv += optind;

	ioctl(1, TIOCGETP, &mode);
	ioctl(1, TIOCGETD, &ldisc);
	ioctl(1, TIOCGETC, &tc);
	ioctl(1, TIOCLGET, &lmode);
	ioctl(1, TIOCGLTC, &ltc);
	ioctl(1, TIOCGWINSZ, &win);
	if (ioctl(1, TIOCMGET, &mstate) < 0) {
		if (errno == ENOTTY)
			mstate = -1;
		else
			warn("TIOCMGET");
	}
	nmstate = mstate;

	if (argc == 0) {
		prmodes(fmt);
		exit(0);
	}
	arg = argv[1];
	if (argc == 1 && arg != 0 && (strcmp(arg, "everything") == 0 ||
            strcmp(arg, "all") == 0)) {
		prmodes(2);
		exit(0);
	}

	while (argc-- > 0) {
		arg = *argv++;
		if (eq("new")){
			ldisc = NTTYDISC;
			if (ioctl(1, TIOCSETD, &ldisc)<0)
				perror("ioctl");
			continue;
		}
		if (eq("newcrt")){
			ldisc = NTTYDISC;
			lmode &= ~LPRTERA;
			lmode |= LCRTBS|LCTLECH;
			if (mode.sg_ospeed >= B1200)
				lmode |= LCRTERA|LCRTKIL;
			if (ioctl(1, TIOCSETD, &ldisc)<0)
				perror("ioctl");
			continue;
		}
		if (eq("crt")){
			lmode &= ~LPRTERA;
			lmode |= LCRTBS|LCTLECH;
			if (mode.sg_ospeed >= B1200)
				lmode |= LCRTERA|LCRTKIL;
			continue;
		}
		if (eq("old")){
			ldisc = 0;
			if (ioctl(1, TIOCSETD, &ldisc)<0)
				perror("ioctl");
			continue;
		}
		if (eq("sane")){
                        mode.sg_flags &= ~CBREAK;
                        mode.sg_flags |= ECHO | CRMOD | XTABS;
			continue;
		}
		if (eq("dec")){
			mode.sg_erase = 0177;
			mode.sg_kill = CTRL('u');
			tc.t_intrc = CTRL('c');
			ldisc = NTTYDISC;
			lmode &= ~LPRTERA;
			lmode |= LCRTBS|LCTLECH|LDECCTQ;
			if (mode.sg_ospeed >= B1200)
				lmode |= LCRTERA|LCRTKIL;
			if (ioctl(1, TIOCSETD, &ldisc)<0)
				perror("ioctl");
			continue;
		}
		for (sp = special; sp->name; sp++)
			if (eq(sp->name)) {
				if (argc-- == 0)
					goto done;
				arg2 = *argv++;
				if (strcmp(arg2, "undef") == 0)
					*sp->cp = 0377;
				else if (*arg2 == '^') {
					arg2++;
					*sp->cp = (*arg2 == '?') ?
					    0177 : *arg2 & 037;
                                } else
					*sp->cp = *arg2;
				goto cont;
			}
		if (eq("flushout")) {
			ioctl(1, TIOCFLUSH, &zero);
			continue;
		}
		if (eq("hup")) {
			ioctl(1, TIOCHPCL, NULL);
			continue;
		}
		if (eq("rows")) {
			if (argc-- == 0)
				goto done;
			win.ws_row = atoi(*argv++);
		}
		if (eq("cols") || eq("columns")) {
			if (argc-- == 0)
				goto done;
			win.ws_col = atoi(*argv++);
		}
		if (eq("size")) {
			printf("%d %d\n", win.ws_row, win.ws_col);
			exit(0);
		}
		for(i=0; speeds[i].string; i++)
			if(eq(speeds[i].string)) {
				mode.sg_ispeed = mode.sg_ospeed = speeds[i].bval;
				goto cont;
			}
		if (eq("speed")) {
			for(i=0; speeds[i].string; i++)
				if (mode.sg_ospeed == speeds[i].bval) {
					printf("%s\n", speeds[i].string);
					exit(0);
				}
			printf("unknown\n");
			exit(1);
		}
		for (i = 0; modes[i].string; i++) {
			if (eq(modes[i].string)) {
				mode.sg_flags &= ~modes[i].reset;
				mode.sg_flags |= modes[i].set;
                                goto cont;
			}
		}
		for (i = 0; lmodes[i].string; i++) {
			if (eq(lmodes[i].string)) {
				lmode &= ~lmodes[i].reset;
				lmode |= lmodes[i].set;
				goto cont;
			}
		}
		for (i = 0; mmodes[i].string; i++) {
			if (eq(mmodes[i].string)) {
				nmstate &= ~mmodes[i].reset;
				nmstate |= mmodes[i].set;
				goto cont;
			}
		}
		if(arg)
			fprintf(stderr,"unknown mode: %s\n", arg);
cont:
		;
	}
done:
	ioctl(1, TIOCSETN, &mode);
	ioctl(1, TIOCSETC, &tc);
	ioctl(1, TIOCSLTC, &ltc);
	ioctl(1, TIOCLSET, &lmode);
	ioctl(1, TIOCSWINSZ, &win);
	if (mstate != -1 && nmstate != mstate) {
		if (ioctl(1, TIOCMSET, &nmstate) < 0)
			warn("TIOCMSET");
	}
}

eq(string)
char *string;
{
	if (!arg)
		return(0);
	if (strcmp(arg, string))
		return(0);
	arg = 0;
	return(1);
}

#define	lpit(what,str) \
	if (all || (lmode & what)) { \
		fprintf(stderr, str + ((lmode & what) != 0)); any++; \
	}

prmodes(all)
	int	all;		/* 0 for short display, !0 for long display */
{
	register m;
	int any, i;
	register struct special *sp;

#ifdef NETLDISC
	if (ldisc==NETLDISC)
		fprintf(stderr, "net discipline, ");
	else
#endif
	if (ldisc==NTTYDISC)
		fprintf(stderr, "new tty, ");
	else if (ldisc == 0)
		fprintf(stderr, "old tty, ");
	else
		fprintf(stderr, "discipline %d, ");

	if(mode.sg_ispeed != mode.sg_ospeed) {
		fprintf(stderr,"input speed %u baud", speeds[mode.sg_ispeed].baud);
		fprintf(stderr,"output speed %u baud", speeds[mode.sg_ospeed].baud);
	} else
		fprintf(stderr,"speed %u baud", speeds[mode.sg_ispeed].baud);

	if (all)
		fprintf(stderr, ", %d rows, %d columns", win.ws_row, win.ws_col);

	fprintf(stderr, all ? "\n" : "; ");
	m = mode.sg_flags;
	if (all) {
		if(m & EVENP)	fprintf(stderr,"even ");
		if(m & ODDP)	fprintf(stderr,"odd ");
	}
	if (all || m & RAW)
		fprintf(stderr, "-raw " + ((m & RAW) != 0));
	if (all || (m & CRMOD) == 0)
		fprintf(stderr, "-nl " + ((m & CRMOD) == 0));
	if (all || (m & ECHO) == 0)
		fprintf(stderr, "-echo " + ((m & ECHO) != 0));
	if (all || (m & TANDEM))
		fprintf(stderr, "-tandem " + ((m & TANDEM) != 0));
	fprintf(stderr, "-tabs "+((m & XTABS) != XTABS));
	if (all || (m & CBREAK))
		fprintf(stderr, "-cbreak " + ((m & CBREAK) != 0));
	lpit(LRTSCTS, "-rtscts ");
	if (all) {
		fputc('\n', stderr);
		if (mstate != -1) {
			fprintf(stderr, "modem control: ");
			fprintf(stderr, "-dcd " + ((mstate & TIOCM_CD) != 0));
			fprintf(stderr, "-dsr " + ((mstate & TIOCM_DSR) != 0));
			fprintf(stderr, "-dtr " + ((mstate & TIOCM_DTR) != 0));
			fprintf(stderr, "-cts " + ((mstate & TIOCM_CTS) != 0));
			fprintf(stderr, "-rts " + ((mstate & TIOCM_RTS) != 0));
                        fputc('\n', stderr);
		}
	}
	if (ldisc == NTTYDISC) {
		int newcrt = (lmode & (LCTLECH|LCRTBS)) == (LCTLECH|LCRTBS) &&
		    (lmode & (LCRTERA|LCRTKIL)) ==
		      ((mode.sg_ospeed > B300) ? LCRTERA|LCRTKIL : 0);
		int nothing = 1;
		if (newcrt) {
			if (all)
				fprintf(stderr, "crt: (crtbs crterase crtkill ctlecho) ");
			else
				fprintf(stderr, "crt ");
			any++;
		} else {
			lpit(LCRTBS, "-crtbs ");
			lpit(LCRTERA, "-crterase ");
			lpit(LCRTKIL, "-crtkill ");
			lpit(LCTLECH, "-ctlecho ");
			lpit(LPRTERA, "-prterase ");
		}
		lpit(LTOSTOP, "-tostop ");
		if (all) {
			fputc('\n', stderr);
			any = 0;
			nothing = 0;
		}
		lpit(LFLUSHO, "-flusho ");
		lpit(LMDMBUF, "-mdmbuf ");
		lpit(LLITOUT, "-litout ");
		lpit(LPASS8, "-pass8 ");
		lpit(LNOHANG, "-nohang ");
		if (any) {
			fputc('\n', stderr);
			any = 0;
			nothing = 0;
		}
		lpit(LPENDIN, "-pendin ");
		lpit(LDECCTQ, "-decctlq ");
		lpit(LNOFLSH, "-noflsh ");
		if (any || nothing)
			fputc('\n', stderr);
	} else if (!all)
		fputc('\n', stderr);

	if (all) {
		for (i = 0, sp = special; i < 9; i++, sp++)
			fprintf(stderr, Nfmt, sp->name);
		fputc('\n', stderr);
		for (i = 0, sp = special; i < 9; i++, sp++)
			pit(sp);
		fputc('\n', stderr);
		for (i = 9, sp = &special[9]; sp->name; i++, sp++)
			fprintf(stderr, Nfmt, sp->name);
		fputc('\n', stderr);
		for (i = 9, sp = &special[9]; sp->name; i++, sp++)
			pit(sp);
		fputc('\n', stderr);
	}
}

pit(sp)
	struct special *sp;
{
	register int	c = *sp->cp & 0xff;
	char	junk[6];
	register char *p = junk;

	if (c == 0xff) {
		fprintf(stderr, Nfmt, "<undef>");
		return;
	}
	if (c & 0200) {
		*p++ = 'M';
		*p++ = '-';
		c &= ~ 0200;
	}
	if (c == 0177) {
		*p++ = '^';
		*p++ = '?';
	}
	else if (c < ' ') {
		*p++ = '^';
		*p++ = c += '@';
	}
	*p++ = '\0';
	fprintf(stderr, Nfmt, junk);
}
