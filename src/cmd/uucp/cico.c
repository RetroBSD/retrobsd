#include <signal.h>
#include "uucp.h"
#include <setjmp.h>
#ifdef	USG
#include <termio.h>
#endif
#ifndef	USG
#include <sgtty.h>
#endif
#ifdef BSDTCP
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif
#include <sys/stat.h>
#include "uust.h"
#include "uusub.h"

#if defined(VMS) && defined(BSDTCP)
#define NOGETPEER
#endif

#ifdef BSD2_9
#define NOGETPEER
#endif

jmp_buf Sjbuf;
jmp_buf Pipebuf;

/*  call fail text  */
char *Stattext[] = {
	"",
	"BAD SYSTEM",
	"WRONG TIME TO CALL",
	"SYSTEM LOCKED",
	"NO DEVICE",
	"CALL FAILED",
	"LOGIN FAILED",
	"BAD SEQUENCE"
};

/*  call fail codes  */
int Stattype[] = {
	0,
	0,
	SS_WRONGTIME,
	0,
	SS_NODEVICE,
	SS_FAIL,
	SS_FAIL,
	SS_BADSEQ
};

				/* Arguments to setdebug():		     */
#define DBG_TEMP  0		/*   Create a temporary audit file	     */
#define DBG_PERM  1		/*   Create a permanent audit file	     */
#define DBG_CLEAN 2		/*   Cleanup, discard temp file		     */

int ReverseRole = 0;
int Role = SLAVE;
int onesys = 0;
int turntime = 30 * 60;	/* 30 minutes expressed in seconds */
char *ttyn = NULL;
extern int LocalOnly;
extern int errno;
extern char MaxGrade, DefMaxGrade;
extern char Myfullname[];

#ifdef	USG
struct termio Savettyb;
#endif
#ifndef	USG
struct sgttyb Savettyb;
#endif

static void onintr(int inter);
static void dbg_signal(int inter);
static void setdebug(int parm);
static void timeout(int);
static char *pskip(char *p);

/*
 *	this program is used  to place a call to a
 *	remote machine, login, and copy files between the two machines.
 */
int main(argc, argv)
int argc;
register char *argv[];
{
	register int ret;
	int seq;
	char wkpre[NAMESIZE], file[NAMESIZE];
	char msg[MAXFULLNAME], *q;
	register char *p;
	char rflags[MAXFULLNAME];
#ifdef NOGETPEER
	u_long Hostnumber = 0;
#endif

	strcpy(Progname, "uucico");

#ifdef BSD4_2
	sigsetmask(0L);	/* in case we inherit blocked signals */
#endif
	signal(SIGINT, (sig_t)onintr);
	signal(SIGHUP, (sig_t)onintr);
	signal(SIGQUIT, (sig_t)onintr);
	signal(SIGTERM, (sig_t)onintr);
	signal(SIGPIPE, (sig_t)onintr);	/* 4.1a tcp-ip stupidity */
	signal(SIGFPE, (sig_t)dbg_signal);
	ret = guinfo(getuid(), User, msg);
	strcpy(Loginuser, User);
	uucpname(Myname);
	ASSERT(ret == 0, "BAD UID", CNULL, ret);

	setbuf (stderr, CNULL);

	rflags[0] = '\0';
	umask(WFMASK);
	strcpy(Rmtname, Myname);
	Ifn = Ofn = -1;
	while(argc>1 && argv[1][0] == '-'){
		switch(argv[1][1]){
		case 'd':
			Spool = &argv[1][2];
			break;
		case 'g':
		case 'p':
			MaxGrade = DefMaxGrade = argv[1][2];
			break;
		case 'r':
			Role = atoi(&argv[1][2]);
			break;
		case 'R':
			ReverseRole++;
			Role = MASTER;
			break;
		case 's':
			strncpy(Rmtname, &argv[1][2], MAXBASENAME);
			Rmtname[MAXBASENAME] = '\0';
			if (Rmtname[0] != '\0')
				onesys = 1;
			break;
		case 'x':
			Debug = atoi(&argv[1][2]);
			if (Debug <= 0)
				Debug = 1;
			strcat(rflags, argv[1]);
			break;
		case 't':
			turntime = atoi(&argv[1][2])*60;/* minutes to seconds */
			break;
		case 'L':	/* local calls only */
			LocalOnly++;
			break;
#ifdef NOGETPEER
		case 'h':
			Hostnumber = inet_addr(&argv[1][2]);
			break;
#endif
		default:
			printf("unknown flag %s (ignored)\n", argv[1]);
			break;
		}
		--argc;  argv++;
	}

	while (argc > 1) {
		fprintf(stderr, "unknown argument %s (ignored)\n", argv[1]);
		--argc; argv++;
	}

	if (Debug && Role == MASTER)
		chkdebug();

	/* Try to run as uucp */
	setgid(getegid());
	setuid(geteuid());
#ifdef	TIOCNOTTY
	/*
	 * detach uucico from controlling terminal
	 * to defend against rlogind sending us a SIGKILL (!!!)
	 */
	if (Role == MASTER && (ret = open("/dev/tty", 2)) >= 0) {
		ioctl(ret, TIOCNOTTY, STBNULL);
		close(ret);
	}
#endif
	if (getpgrp(0) == 0) { /* We have no controlling terminal */
		setpgrp();
	}

	ret = subchdir(Spool);
	ASSERT(ret >= 0, "CHDIR FAILED", Spool, ret);
	strcpy(Wrkdir, Spool);

	if (Debug) {
		setdebug ((Role == SLAVE) ? DBG_TEMP : DBG_PERM);
		if (Debug > 0)
			logent ("Local Enabled", "DEBUG");
	}

	/*
	 * First time through: If we're the slave, do initial checking.
	 */
	if (Role == SLAVE) {
		/* check for /etc/nologin */
		if (access(NOLOGIN, 0) == 0) {
			logent(NOLOGIN, "UUCICO SHUTDOWN");
			if (Debug > 4)
				logent("DEBUGGING", "continuing anyway");
			else
				cleanup(1);
		}
		Ifn = 0;
		Ofn = 1;
#ifdef	TCPIP
		/*
		 * Determine if we are on TCPIP
		 */
		if (isatty(Ifn) < 0) {
			IsTcpIp = 1;
			DEBUG(4, "TCPIP connection -- ioctl-s disabled\n", CNULL);
		} else
			IsTcpIp = 0;
#endif
		/* initial handshake */
		onesys = 1;
		if (!IsTcpIp) {
#ifdef	USG
			ret = ioctl(Ifn, TCGETA, &Savettyb);
			Savettyb.c_cflag = (Savettyb.c_cflag & ~CS8) | CS7;
			Savettyb.c_oflag |= OPOST;
			Savettyb.c_lflag |= (ISIG|ICANON|ECHO);
#else
			ret = ioctl(Ifn, TIOCGETP, &Savettyb);
			Savettyb.sg_flags |= ECHO;
			Savettyb.sg_flags &= ~RAW;
#endif
			ttyn = ttyname(Ifn);
		}
		fixmode(Ifn);

		/*
		 * Initial Message -- tell them we're here, and who we are.
		 */
		sprintf(msg, "here=%s", Myfullname);
		omsg('S', msg, Ofn);
		signal(SIGALRM, (sig_t)timeout);
		alarm(MAXMSGTIME);
		if (setjmp(Sjbuf)) {
			/* timed out */
			if (!IsTcpIp) {
#ifdef	USG
				ret = ioctl(Ifn, TCSETA, &Savettyb);

#else
				ret = ioctl(Ifn, TIOCSETP, &Savettyb);
#endif
			}
			cleanup(0);
		}
		for (;;) {
			ret = imsg(msg, Ifn);
			if (ret != SUCCESS) {
				alarm(0);
				if (!IsTcpIp) {
#ifdef	USG
					ret = ioctl(Ifn, TCSETA, &Savettyb);
#else
					ret = ioctl(Ifn, TIOCSETP, &Savettyb);
#endif
				}
				cleanup(0);
			}
			if (msg[0] == 'S')
				break;
		}
		alarm(0);
		q = &msg[1];
		p = pskip(q);
		strncpy(Rmtname, q, MAXBASENAME);
		Rmtname[MAXBASENAME] = '\0';

		/*
		 * Now that we know who they are, give the audit file the right
		 * name.
		 */
		setdebug (DBG_PERM);
		DEBUG(4, "sys-%s\n", Rmtname);
		/* The versys will also do an alias on the incoming name */
		if (versys(&Rmtname)) {
			/* If we don't know them, we won't talk to them... */
#ifdef	NOSTRANGERS
			logent(Rmtname, "UNKNOWN HOST");
			omsg('R', "You are unknown to me", Ofn);
			cleanup(0);
#endif
		}
#ifdef BSDTCP
		/* we must make sure they are really who they say they
		 * are. We compare the hostnumber with the number in the hosts
		 * table for the site they claim to be.
		 */
		if (IsTcpIp) {
			struct hostent *hp;
			char *cpnt, *inet_ntoa();
			int fromlen;
			struct sockaddr_in from;
			extern char PhoneNumber[];

#ifdef	NOGETPEER
			from.sin_addr.s_addr = Hostnumber;
			from.sin_family = AF_INET;
#else
			fromlen = sizeof(from);
			if (getpeername(Ifn, &from, &fromlen) < 0) {
				logent(Rmtname, "NOT A TCP CONNECTION");
				omsg('R', "NOT TCP", Ofn);
				cleanup(0);
			}
#endif
			hp = gethostbyaddr(&from.sin_addr,
				sizeof (struct in_addr), from.sin_family);
			if (hp == NULL) {
				/* security break or just old host table? */
				logent(Rmtname, "UNKNOWN IP-HOST Name =");
				cpnt = inet_ntoa(from.sin_addr),
				logent(cpnt, "UNKNOWN IP-HOST Number =");
				sprintf(wkpre, "%s/%s isn't in my host table",
					Rmtname, cpnt);
				omsg('R' ,wkpre ,Ofn);
				cleanup(0);
			}
			if (Debug > 99)
				logent(Rmtname,"Request from IP-Host name =");
			/*
			 * The following is to determine if the name given us by
			 * the Remote uucico matches any of the names
			 * given its network number (remote machine) in our
			 * host table.
			 * We could check the aliases, but that won't work in
			 * all cases (like if you are running the domain
			 * server, where you don't get any aliases). The only
			 * reliable way I can think of that works in ALL cases
			 * is too look up the site in L.sys and see if the
			 * sitename matches what we would call him if we
			 * originated the call.
			 */
			/* PhoneNumber contains the official network name of the 			   host we are checking. (set in versys.c) */
			if (sncncmp(PhoneNumber, hp->h_name, SYSNSIZE) == 0) {
				if (Debug > 99)
					logent(q,"Found in host Tables");
			} else {
				logent(hp->h_name, "FORGED HOSTNAME");
				logent(inet_ntoa(from.sin_addr), "ORIGINATED AT");
				logent(PhoneNumber, "SHOULD BE");
				sprintf(wkpre, "You're not who you claim to be: %s !=  %s", hp->h_name, PhoneNumber);
				omsg('R', wkpre, Ofn);
				cleanup(0);
			}
		}
#endif

		if (mlock(Rmtname)) {
			omsg('R', "LCK", Ofn);
			cleanup(0);
		}
		else if (callback(Loginuser)) {
			signal(SIGINT, SIG_IGN);
			signal(SIGHUP, SIG_IGN);
			omsg('R', "CB", Ofn);
			logent("CALLBACK", "REQUIRED");
			/*  set up for call back  */
			systat(Rmtname, SS_CALLBACK, "CALLING BACK");
			gename(CMDPRE, Rmtname, 'C', file);
			close(creat(subfile(file), 0666));
			xuucico(Rmtname);
			cleanup(0);
		}
		seq = 0;
		while (*p == '-') {
			q = pskip(p);
			switch(*(++p)) {
			case 'x':
				if (Debug == 0) {
					Debug = atoi(++p);
					if (Debug <= 0)
						Debug = 1;
					setdebug(DBG_PERM);
					if (Debug > 0)
						logent("Remote Enabled", "DEBUG");
				} else {
					DEBUG(1, "Remote debug request ignored\n",
					   CNULL);
				}
				break;
			case 'Q':
				seq = atoi(++p);
				break;
			case 'p':
				MaxGrade = DefMaxGrade = *++p;
				DEBUG(4, "MaxGrade set to %c\n", MaxGrade);
				break;
			case 'v':
				if (strncmp(p, "grade", 5) == 0) {
					p += 6;
					MaxGrade = DefMaxGrade = *p++;
					DEBUG(4, "MaxGrade set to %c\n", MaxGrade);
				}
				break;
			default:
				break;
			}
			p = q;
		}
		if (callok(Rmtname) == SS_BADSEQ) {
			logent("BADSEQ", "PREVIOUS");
			omsg('R', "BADSEQ", Ofn);
			cleanup(0);
		}
#ifdef GNXSEQ
		if ((ret = gnxseq(Rmtname)) == seq) {
			omsg('R', "OK", Ofn);
			cmtseq();
		} else {
#else
		if (seq == 0)
			omsg('R', "OK", Ofn);
		else {
#endif
			systat(Rmtname, Stattype[7], Stattext[7]);
			logent("BAD SEQ", "FAILED HANDSHAKE");
#ifdef GNXSEQ
			ulkseq();
#endif
			omsg('R', "BADSEQ", Ofn);
			cleanup(0);
		}
		if (ttyn != NULL)
			chmod(ttyn, 0600);
	}

loop:
	if(setjmp(Pipebuf)) {	/* come here on SIGPIPE	*/
		clsacu();
		close(Ofn);
		close(Ifn);
		Ifn = Ofn = -1;
		rmlock(CNULL);
		sleep(3);
	}
	if (!onesys) {
		ret = gnsys(Rmtname, Spool, CMDPRE);
		setdebug(DBG_PERM);
		if (ret == FAIL)
			cleanup(100);
		if (ret == SUCCESS)
			cleanup(0);
	} else if (Role == MASTER && callok(Rmtname) != 0) {
		logent("SYSTEM STATUS", "CAN NOT CALL");
		cleanup(0);
	}

	sprintf(wkpre, "%c.%.*s", CMDPRE, SYSNSIZE, Rmtname);

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	if (Role == MASTER) {
		/* check for /etc/nologin */
		if (access(NOLOGIN, 0) == 0) {
			logent(NOLOGIN, "UUCICO SHUTDOWN");
			if (Debug > 4)
				logent("DEBUGGING", "continuing anyway");
			else
				cleanup(1);
		}
		/*  master part */
		signal(SIGHUP, SIG_IGN);
		if (Ifn != -1 && Role == MASTER) {
			write(Ofn, EOTMSG, strlen(EOTMSG));
			clsacu();
			close(Ofn);
			close(Ifn);
			Ifn = Ofn = -1;
			rmlock(CNULL);
			sleep(3);
		}
		sprintf(msg, "call to %s ", Rmtname);
		if (mlock(Rmtname) != SUCCESS) {
			logent(msg, "LOCKED");
			US_SST(us_s_lock);
			goto next;
		}
		Ofn = Ifn = conn(Rmtname);
		if (Ofn < 0) {
			if (Ofn != CF_TIME)
				logent(msg, _FAILED);
			/* avoid excessive 'wrong time' info */
			if (Stattype[-Ofn] != SS_WRONGTIME){
				systat(Rmtname, Stattype[-Ofn], Stattext[-Ofn]);
				US_SST(-Ofn);
				UB_SST(-Ofn);
			}
			goto next;
		} else {
			logent(msg, "SUCCEEDED");
			US_SST(us_s_cok);
			UB_SST(ub_ok);
		}
#ifdef	TCPIP
		/*
		 * Determine if we are on TCPIP
		 */
		if (isatty(Ifn) == 0) {
			IsTcpIp = 1;
			DEBUG(4, "TCPIP connection -- ioctl-s disabled\n", CNULL);
		} else
			IsTcpIp = 0;
#endif

		if (setjmp(Sjbuf))
			goto next;
		signal(SIGALRM, (sig_t)timeout);
		alarm(2 * MAXMSGTIME);
		for (;;) {
			ret = imsg(msg, Ifn);
			if (ret != 0) {
				alarm(0);
				logent("imsg 1", _FAILED);
				goto Failure;
			}
			if (msg[0] == 'S')
				break;
		}
		alarm(MAXMSGTIME);
#ifdef GNXSEQ
		seq = gnxseq(Rmtname);
#else
		seq = 0;
#endif
		if (MaxGrade != '\177') {
			DEBUG(2, "Max Grade this transfer is %c\n", MaxGrade);
			sprintf(msg, "%s -Q%d -p%c -vgrade=%c %s",
				Myname, seq, MaxGrade, MaxGrade, rflags);
		} else
			sprintf(msg, "%s -Q%d %s", Myname, seq, rflags);
		omsg('S', msg, Ofn);
		for (;;) {
			ret = imsg(msg, Ifn);
			DEBUG(4, "msg-%s\n", msg);
			if (ret != SUCCESS) {
				alarm(0);
#ifdef GNXSEQ
				ulkseq();
#endif
				logent("imsg 2", _FAILED);
				goto Failure;
			}
			if (msg[0] == 'R')
				break;
		}
		alarm(0);
		if (msg[1] == 'B') {
			/* bad sequence */
			logent("BAD SEQ", "FAILED HANDSHAKE");
			US_SST(us_s_hand);
			systat(Rmtname, SS_BADSEQ, Stattext[SS_BADSEQ]);
#ifdef GNXSEQ
			ulkseq();
#endif
			goto next;
		}
		if (strcmp(&msg[1], "OK") != SAME)  {
			logent(&msg[1], "FAILED HANDSHAKE");
			US_SST(us_s_hand);
#ifdef GNXSEQ
			ulkseq();
#endif
			systat(Rmtname, SS_INPROGRESS,
				strcmp(&msg[1], "CB") == SAME?
				"AWAITING CALLBACK": "FAILED HANDSHAKE");
			goto next;
		}
#ifdef GNXSEQ
		cmtseq();
#endif
	}
	DEBUG(1, "Rmtname %s, ", Rmtname);
	DEBUG(1, "Role %s,  ", Role ? "MASTER" : "SLAVE");
	DEBUG(1, "Ifn - %d, ", Ifn);
	DEBUG(1, "Loginuser - %s\n", Loginuser);

	ttyn = ttyname(Ifn);

	alarm(MAXMSGTIME);
	if ((ret = setjmp(Sjbuf)))
		goto Failure;
	ret = startup(Role);
	alarm(0);
	if (ret != SUCCESS) {
		logent("startup", _FAILED);
Failure:
		US_SST(us_s_start);
		systat(Rmtname, SS_FAIL, ret > 0 ? "CONVERSATION FAILED" :
			"STARTUP FAILED");
		goto next;
	} else {
		if (ttyn != NULL) {
			char startupmsg[BUFSIZ];
			extern int linebaudrate;
			sprintf(startupmsg, "startup %s %d baud", &ttyn[5],
				linebaudrate);
			logent(startupmsg, "OK");
		} else
			logent("startup", "OK");
		US_SST(us_s_gress);
		systat(Rmtname, SS_INPROGRESS, "TALKING");
		ret = cntrl(Role, wkpre);
		DEBUG(1, "cntrl - %d\n", ret);
		signal(SIGINT, SIG_IGN);
		signal(SIGHUP, SIG_IGN);
		signal(SIGALRM, (sig_t)timeout);
		if (ret == SUCCESS) {
			logent("conversation complete", "OK");
			US_SST(us_s_ok);
			rmstat(Rmtname);

		} else {
			logent("conversation complete", _FAILED);
			US_SST(us_s_cf);
			systat(Rmtname, SS_FAIL, "CONVERSATION FAILED");
		}
		alarm(MAXMSGTIME);
		DEBUG(4, "send OO %d,", ret);
		if (!setjmp(Sjbuf)) {
			for (;;) {
				omsg('O', "OOOOO", Ofn);
				ret = imsg(msg, Ifn);
				if (ret != 0)
					break;
				if (msg[0] == 'O')
					break;
			}
		}
		alarm(0);
		clsacu();
		rmlock(CNULL);
	}
next:
	if (!onesys) {
		goto loop;
	}
	cleanup(0);
}

#ifndef	USG
struct sgttyb Hupvec;
#endif

/*
 *	cleanup and exit with "code" status
 */
void cleanup(code)
register int code;
{
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	rmlock(CNULL);
	sleep(5);			/* Wait for any pending output	  */
	clsacu();
	logcls();
	if (Role == SLAVE) {
		if (!IsTcpIp) {
#ifdef USG
			Savettyb.c_cflag |= HUPCL;
			(void) ioctl(0, TCSETA, &Savettyb);
#else
			(void) ioctl(0, TIOCHPCL, STBNULL);
#ifdef TIOCSDTR
			(void) ioctl(0, TIOCCDTR, STBNULL);
			sleep(2);
			(void) ioctl(0, TIOCSDTR, STBNULL);
#else
			(void) ioctl(0, TIOCGETP, &Hupvec);
			Hupvec.sg_ispeed = B0;
			Hupvec.sg_ospeed = B0;
			(void) ioctl(0, TIOCSETP, &Hupvec);
#endif
			sleep(2);
			(void) ioctl(0, TIOCSETP, &Savettyb);
			/* make *sure* exclusive access is off */
			(void) ioctl(0, TIOCNXCL, STBNULL);
#endif
		}
		if (ttyn != NULL)
			chmod(ttyn, 0600);
	}
	if (Ofn != -1) {
		if (Role == MASTER)
			write(Ofn, EOTMSG, strlen(EOTMSG));
		close(Ifn);
		close(Ofn);
	}
#ifdef DIALINOUT
	/* reenable logins on dialout */
	reenable();
#endif
	if (code == 0)
		xuuxqt();
	else
		DEBUG(1, "exit code %d\n", code);
	setdebug (DBG_CLEAN);
	exit(code);
}

/*
 *	on interrupt - remove locks and exit
 */
void onintr(inter)
register int inter;
{
	char str[30];
	signal(inter, SIG_IGN);
	sprintf(str, "SIGNAL %d", inter);
	logent(str, "CAUGHT");
	US_SST(us_s_intr);
	if (*Rmtname && strncmp(Rmtname, Myname, MAXBASENAME))
		systat(Rmtname, SS_FAIL, str);
	if (inter == SIGPIPE && !onesys)
		longjmp(Pipebuf, 1);
	cleanup(inter);
}

/*
 * Catch a special signal
 * (SIGFPE, ugh), and toggle debugging between 0 and 30.
 * Handy for looking in on long running uucicos.
 */
void dbg_signal()
{
	Debug = (Debug == 0) ? 30 : 0;
	setdebug(DBG_PERM);
	if (Debug > 0)
		logent("Signal Enabled", "DEBUG");
}

/*
 * Check debugging requests, and open RMTDEBUG audit file if necessary. If an
 * audit file is needed, the parm argument indicates how to create the file:
 *
 *	DBG_TEMP  - Open a temporary file, with filename = RMTDEBUG/pid.
 *	DBG_PERM  - Open a permanent audit file, filename = RMTDEBUG/Rmtname.
 *		    If a temp file already exists, it is mv'ed to be permanent.
 *	DBG_CLEAN - Cleanup; unlink temp files.
 *
 * Restrictions - this code can only cope with one open debug file at a time.
 * Each call creates a new file; if an old one of the same name exists it will
 * be overwritten.
 */
void setdebug(parm)
int parm;
{
	char buf[BUFSIZ];		/* Buffer for building filenames     */
	static char *temp = NULL;	/* Ptr to temporary file name	     */
	static int auditopen = 0;	/* Set to 1 when we open a file	     */
	struct stat stbuf;		/* File status buffer		     */

	/*
	 * If movement or cleanup of a temp file is indicated, we do it no
	 * matter what.
	 */
	if (temp != CNULL && parm == DBG_PERM) {
		sprintf(buf, "%s/%s", RMTDEBUG, Rmtname);
		unlink(buf);
		if (link(temp, buf) != 0) {
			Debug = 0;
			assert("RMTDEBUG LINK FAIL", temp, errno);
			cleanup(1);
		}
		parm = DBG_CLEAN;
	}
	if (parm == DBG_CLEAN) {
		if (temp != CNULL) {
			unlink(temp);
			free(temp);
			temp = CNULL;
		}
		return;
	}

	if (Debug == 0)
		return;		/* Gotta be in debug to come here.   */

	/*
	 * If we haven't opened a file already, we can just return if it's
	 * alright to use the stderr we came in with. We can if:
	 *
	 *	Role == MASTER, and Stderr is a regular file, a TTY or a pipe.
	 *
	 * Caution: Detecting when stderr is a pipe is tricky, because the 4.2
	 * man page for fstat(2) disagrees with reality, and System V leaves it
	 * undefined, which means different implementations act differently.
	 */
	if (!auditopen && Role == MASTER) {
		if (isatty(fileno(stderr)))
			return;
		else if (fstat(fileno(stderr), &stbuf) == 0) {
#ifdef USG
			/* Is Regular File or Fifo   */
			if ((stbuf.st_mode & 0060000) == 0)
				return;
#else
#ifdef BSD4_2
					/* Is Regular File */
			if ((stbuf.st_mode & S_IFMT) == S_IFREG ||
			    stbuf.st_mode == 0)		/* Is a pipe */
				return;
#else
					 /* Is Regular File or Pipe  */
			if ((stbuf.st_mode & S_IFMT) == S_IFREG)
				return;
#endif
#endif
		}
	}

	/*
	 * We need RMTDEBUG directory to do auditing. If the file doesn't exist,
	 * then we forget about debugging; if it exists but has improper owner-
	 * ship or modes, we gripe about it in ERRLOG.
	 */
	if (stat(RMTDEBUG, &stbuf) != SUCCESS) {
		Debug = 0;
		return;
	}
	if ((geteuid() != stbuf.st_uid) ||	  	/* We must own it    */
	    ((stbuf.st_mode & 0170700) != 040700)) {	/* Directory, rwx    */
		Debug = 0;
		assert("INVALID RMTDEBUG DIRECTORY:", RMTDEBUG, stbuf.st_mode);
		return;
	}

	if (parm == DBG_TEMP) {
		sprintf(buf, "%s/%d", RMTDEBUG, getpid());
		temp = (char *)malloc(strlen (buf) + 1);
		if (temp == CNULL) {
			Debug = 0;
			assert("RMTDEBUG MALLOC ERROR:", temp, errno);
			cleanup(1);
		}
		strcpy(temp, buf);
	} else
		sprintf(buf, "%s/%s", RMTDEBUG, Rmtname);

	unlink(buf);
	if (freopen(buf, "w", stderr) != stderr) {
		Debug = 0;
		assert("FAILED RMTDEBUG FILE OPEN:", buf, errno);
		cleanup(1);
	}
	setbuf(stderr, CNULL);
	auditopen = 1;
}

/*
 *	catch SIGALRM routine
 */
void timeout()
{
	extern int HaveSentHup;
	if (!HaveSentHup) {
		logent(Rmtname, "TIMEOUT");
		if (*Rmtname && strncmp(Rmtname, Myname, MAXBASENAME)) {
			US_SST(us_s_tmot);
			systat(Rmtname, SS_FAIL, "TIMEOUT");
		}
	}
	longjmp(Sjbuf, 1);
}

static char *
pskip(p)
register char *p;
{
	while(*p && *p != ' ')
		++p;
	while(*p && *p == ' ')
		*p++ = 0;
	return p;
}
