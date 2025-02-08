/*
 * Here are various dialers to establish the machine-machine connection.
 * conn.c/condevs.c was glued together by Mike Mitchell.
 * The dialers were supplied by many people, to whom we are grateful.
 *
 * ---------------------------------------------------------------------
 * NOTE:
 * There is a bug that occurs at least on PDP11s due to a limitation of
 * setjmp/longjmp.   If the routine that does a setjmp is interrupted
 * and longjmp-ed to,  it loses its register variables (on a pdp11).
 * What works is if the routine that does the setjmp
 * calls a routine and it is the *subroutine* that is interrupted.
 *
 * Anyway, in conclusion, condevs.c is plagued with register variables
 * that are used inside
 * 	if (setjmp(...)) {
 * 		....
 * 	}
 *
 * THE FIX: Don't declare variables to be register
 */

#include "condevs.h"

static int dircls(int fd);
static int nullopn(char *telno, char *flds[], struct Devices *dev);
static int Acuopn(char *flds[]);

struct condev condevs[] = {
	{ "DIR", "direct", diropn, nullopn, dircls },
#ifdef DATAKIT
	{ "DK", "datakit", dkopn, nullopn, nulldev },
#endif
#ifdef PNET
	{ "PNET", "pnet", pnetopn, nullopn, nulldev },
#endif
#ifdef	UNETTCP
	{ "TCP", "TCP", unetopn, nullopn, unetcls },
#endif
#ifdef BSDTCP
	{ "TCP", "TCP", bsdtcpopn, nullopn, bsdtcpcls },
#endif
#ifdef MICOM
	{ "MICOM", "micom", micopn, nullopn, miccls },
#endif
#ifdef DN11
	{ "ACU", "dn11", Acuopn, dnopn, dncls },
#endif
#ifdef HAYES
	{ "ACU", "hayes", Acuopn, hyspopn, hyscls },
	{ "ACU", "hayespulse", Acuopn, hyspopn, hyscls },
	{ "ACU", "hayestone", Acuopn, hystopn, hyscls },
	{ "WATS", "hayestone", Acuopn, hystopn, hyscls },
#endif
#ifdef HAYES2400
	{ "ACU", "hayes2400", Acuopn, hyspopn24, hyscls24 },
	{ "ACU", "hayes2400pulse", Acuopn, hyspopn24, hyscls24 },
	{ "ACU", "hayes2400tone", Acuopn, hystopn24, hyscls24 },
#endif
#ifdef HAYESQ	/* a version of hayes that doesn't use result codes */
	{ "ACU", "hayesq", Acuopn, hysqpopn, hysqcls },
	{ "ACU", "hayesqpulse", Acuopn, hysqpopn, hysqcls },
	{ "ACU", "hayesqtone", Acuopn, hysqtopn, hysqcls },
#endif
#ifdef CDS224
	{ "ACU", "cds224", Acuopn, cdsopn224, cdscls224},
#endif
#ifdef NOVATION
	{ "ACU", "novation", Acuopn, novopn, novcls},
#endif
#ifdef DF02
	{ "ACU", "DF02", Acuopn, df2opn, df2cls },
#endif
#ifdef DF112
	{ "ACU", "DF112P", Acuopn, df12popn, df12cls },
	{ "ACU", "DF112T", Acuopn, df12topn, df12cls },
#endif
#ifdef VENTEL
	{ "ACU", "ventel", Acuopn, ventopn, ventcls },
#endif
#ifdef PENRIL
	{ "ACU", "penril", Acuopn, penopn, pencls },
#endif
#ifdef VADIC
	{ "ACU", "vadic", Acuopn, vadopn, vadcls },
#endif
#ifdef VA212
	{ "ACU", "va212", Acuopn, va212opn, va212cls },
#endif
#ifdef VA811S
	{ "ACU", "va811s", Acuopn, va811opn, va811cls },
#endif
#ifdef VA820
	{ "ACU", "va820", Acuopn, va820opn, va820cls },
	{ "WATS", "va820", Acuopn, va820opn, va820cls },
#endif
#ifdef RVMACS
	{ "ACU", "rvmacs", Acuopn, rvmacsopn, rvmacscls },
#endif
#ifdef VMACS
	{ "ACU", "vmacs", Acuopn, vmacsopn, vmacscls },
#endif
#ifdef SYTEK
	{ "SYTEK", "sytek", sykopn, nullopn, sykcls },
#endif
#ifdef ATT2224
	{ "ACU", "att", Acuopn, attopn, attcls },
#endif


	/* Insert new entries before this line */
	{ NULL, NULL, NULL, NULL, NULL }
};

/*
 *	nullopn		a null device (returns CF_DIAL)
 */
int nullopn(char *telno, char *flds[], struct Devices *dev)
{
	return CF_DIAL;
}

/*
 *	nulldev		a null device (returns CF_DIAL)
 */
int nulldev()
{
	return CF_DIAL;
}

/*
 *	nodev		a null device (returns CF_NODEV)
 */
int nodev()
{
	return CF_NODEV;
}

/*
 * Generic devices look through L-devices and call the CU_open routines for
 * appropriate devices.  Some things, like the tcp/ip interface, or direct
 * connect, do not use the CU_open entry.  ACUs must search to find the
 * right routine to call.
 */

/*
 *	diropn(flds)	connect to hardware line
 *
 *	return codes:
 *		> 0  -  file number  -  ok
 *		FAIL  -  failed
 */
int diropn(flds)
register char *flds[];
{
	register int dcr, status;
	struct Devices dev;
	char dcname[20];
	FILE *dfp;
#ifdef VMSDTR	/* Modem control on vms(works dtr) */
	int modem_control;
	short iosb[4];
	int sys$qiow();	/* use this for long reads on vms */
	int ret;
	long mode[2];
	modem_control = 0;
#endif
	dfp = fopen(DEVFILE, "r");
	ASSERT(dfp != NULL, "CAN'T OPEN", DEVFILE, 0);
	while ((status = rddev(dfp, &dev)) != FAIL) {
#ifdef VMSDTR	/* Modem control on vms(works dtr) */
		/* If we find MOD in the device type field we go into action */
		if (strcmp(dev.D_type, "MOD") == SAME) {
			modem_control = 1;
		        DEBUG(7, "Setting Modem control to %d",modem_control);
		}
		if (strcmp(flds[F_CLASS], dev.D_class) != SAME)
				continue;
		/*
		 * Modem control on vms(works dtr) Take anything in MOD class.
		 * It probably should work differently anyway so we can have
		 *  multiple hardwired lines.
		 */
		if (!modem_control&&strcmp(flds[F_PHONE], dev.D_line) != SAME)
#else
		if (strcmp(flds[F_CLASS], dev.D_class) != SAME)
			continue;
		if (strcmp(flds[F_PHONE], dev.D_line) != SAME)
#endif
			continue;
		if (mlock(dev.D_line) != FAIL)
			break;
	}
	fclose(dfp);
	if (status == FAIL) {
		logent("DEVICE", "NO");
		return CF_NODEV;
	}

	sprintf(dcname, "/dev/%s", dev.D_line);
	if (setjmp(Sjbuf)) {
		DEBUG(4, "Open timed out\n", CNULL);
		delock(dev.D_line);
		return CF_DIAL;
	}
	signal(SIGALRM, (sig_t)alarmtr);
	/* For PC Pursuit, it could take a while to call back */
	alarm( strcmp(flds[F_LINE], "PCP") ? 10 : MAXMSGTIME*4 );
	getnextfd();
	errno = 0;
        DEBUG(4,"Opening %s\n",dcname);
	dcr = open(dcname, 2); /* read/write */
#ifdef VMSDTR	/* Modem control on vms(works dtr) */
	fflush(stdout);
	if (modem_control) { /* Did we have MOD in the device type field ? */
		/* Sense the current terminal setup and save it */
		if ((ret = sys$qiow(_$EFN,(fd_fab_pointer[dcr]->fab).fab$l_stv,
			IO$_SENSEMODE,iosb,0,0,mode,8,0,0,0,0))
				!= SS$_NORMAL) {
			DEBUG(7, "ret status on sense failed on Modem sense=%x<", ret);
			return CF_DIAL;
		}
		mode[1] |= TT$M_MODEM; /* Or in modem control(DTR) */
		/* Now set the new terminal characteristics */
		/* This is temporary and will go away when we let go of it */
		if ((ret = sys$qiow(_$EFN,(fd_fab_pointer[dcr]->fab).fab$l_stv,
			IO$_SETMODE,iosb,0,0,mode,8,0,0,0,0))
				!= SS$_NORMAL) {
			DEBUG(7, "ret status on sense failed on Modem setup=%x<", ret);
			return CF_DIAL;
		}
	}
#endif
	next_fd = -1;
	alarm(0);
	if (dcr < 0) {
		if (errno == EACCES)
			logent(dev.D_line, "CANT OPEN");
		DEBUG(4, "OPEN FAILED: errno %d\n", errno);
		delock(dev.D_line);
		return CF_DIAL;
	}
	fflush(stdout);
	if (fixline(dcr, dev.D_speed) == FAIL) {
		DEBUG(4, "FIXLINE FAILED\n", CNULL);
		return CF_DIAL;
	}
	strcpy(devSel, dev.D_line);	/* for latter unlock */
	CU_end = dircls;
	return dcr;
}

int dircls(fd)
register int fd;
{
	if (fd > 0) {
		close(fd);
		delock(devSel);
	}
        return 0;
}

/*
 *	open an ACU and dial the number.  The condevs table
 *	will be searched until a dialing unit is found that is free.
 *
 *	return codes:	>0 - file number - o.k.
 *			FAIL - failed
 */
char devSel[20];	/* used for later unlock() */

int Acuopn(flds)
register char *flds[];
{
	char phone[MAXPH+1];
	register struct condev *cd;
	register int fd, acustatus;
	register FILE *dfp;
	struct Devices dev;
	int retval = CF_NODEV;
	char nobrand[MAXPH], *line;

	exphone(flds[F_PHONE], phone);
	if (snccmp(flds[F_LINE], "LOCAL") == SAME)
		line = "ACU";
	else
		line = flds[F_LINE];
	devSel[0] = '\0';
	nobrand[0] = '\0';
	DEBUG(4, "Dialing %s\n", phone);
	dfp = fopen(DEVFILE, "r");
	ASSERT(dfp != NULL, "Can't open", DEVFILE, 0);

	acustatus = 0;	/* none found, none locked */
	while (rddev(dfp, &dev) != FAIL) {
		/*
		 * for each ACU L.sys line, try at most twice
		 * (TRYCALLS) to establish carrier.  The old way tried every
		 * available dialer, which on big sites takes forever!
		 * Sites with a single auto-dialer get one try.
		 * Sites with multiple dialers get a try on each of two
		 * different dialers.
		 * To try 'harder' to connect to a remote site,
		 * use multiple L.sys entries.
		 */
		if (acustatus > TRYCALLS)
			break;
		if (strcmp(flds[F_CLASS], dev.D_class) != SAME)
			continue;
		if (snccmp(line, dev.D_type) != SAME)
			continue;
		if (dev.D_brand[0] == '\0') {
			logent("Acuopn","No 'brand' name on ACU");
			continue;
		}
		for(cd = condevs; cd->CU_meth != NULL; cd++) {
			if (snccmp(line, cd->CU_meth) == SAME) {
				if (snccmp(dev.D_brand, cd->CU_brand) == SAME)
					break;
				strncpy(nobrand, dev.D_brand, sizeof nobrand);
			}
		}

		if (mlock(dev.D_line) == FAIL) {
			acustatus++;
			continue;
		}
		if (acustatus < 1)
			acustatus = 1;	/* has been found */
#ifdef DIALINOUT
#ifdef ALLACUINOUT
		if (1) {
#else
		if (snccmp("inout", dev.D_calldev) == SAME) {
#endif
			if (disable(dev.D_line) == FAIL) {
				delock(dev.D_line);
				continue;
			}
		}  else
			reenable();
#endif

		DEBUG(4, "Using %s\n", cd->CU_brand);
		acustatus++;
		fd = (*(cd->CU_open))(phone, flds, &dev);
		if (fd > 0) {
			CU_end = cd->CU_clos;   /* point CU_end at close func */
			fclose(dfp);
			strcpy(devSel, dev.D_line);   /* save for later unlock() */
			return fd;
		} else
			delock(dev.D_line);
		retval = CF_DIAL;
	}
	fclose(dfp);
	if (acustatus == 0) {
		if (nobrand[0])
			logent(nobrand, "unsupported ACU type");
		else
			logent("L-devices", "No appropriate ACU");
	}
	if (acustatus == 1)
		logent("DEVICE", "NO");
	return retval;
}

/*
 * intervaldelay:  delay execution for numerator/denominator seconds.
 */
#ifdef INTERVALTIMER
#define uucpdelay(num,denom) intervaldelay(num,denom)

void intervaldelay(num,denom)
int num, denom;
{
	struct timeval tv;
	tv.tv_sec = num / denom;
	tv.tv_usec = (num * 1000000L / denom ) % 1000000L;
	(void) select (0, (fd_set*)0, (fd_set*)0, (fd_set*)0, &tv);
}
#endif

/*
 * Sleep in increments of 60ths of second.
 */
#ifdef FASTTIMER
#define uucpdelay(num,denom) nap(60*num/denom)

void nap (time)
register int time;
{
	static int fd;

	if (fd == 0)
		fd = open (FASTTIMER, 0);

	read (fd, 0, time);
}
#endif

#ifdef FTIME
#define uucpdelay(num,denom) ftimedelay(1000*num/denom)

void ftimedelay(n)
{
	static struct timeval loctime;
	register i = loctime.millitm;

	ftime(&loctime);
	while (abs((int)(loctime.millitm - i))<n) ftime(&loctime)
		;
}
#endif

#ifdef BUSYLOOP
#define uucpdelay(num,denom) busyloop(CPUSPEED*num/denom)
#define CPUSPEED 1000000	/* VAX 780 is 1MIPS */
#define	DELAY(n)	{ register long N = (n); while (--N > 0); }

void busyloop(n)
{
	DELAY(n);
}
#endif

void slowrite(fd, str)
register char *str;
{
	DEBUG(6, "slowrite ", CNULL);
	while (*str) {
		DEBUG(6, "%c", *str);
		uucpdelay(1, 10);	/* delay 1/10 second */
		write(fd, str, 1);
		str++;
	}
	DEBUG(6, "\n", CNULL);
}

#define BSPEED B150

/*
 *	send a break
 */
void genbrk(fn, bnulls)
register int fn, bnulls;
{
#ifdef	USG
	if (ioctl(fn, TCSBRK, STBNULL) < 0) {
		DEBUG(5, "break TCSBRK %s\n", strerror(errno));
        }
#else
# ifdef	TIOCSBRK
	if (ioctl(fn, TIOCSBRK, STBNULL) < 0) {
		DEBUG(5, "break TIOCSBRK %s\n", strerror(errno));
        }
# ifdef	TIOCCBRK
	uucpdelay(bnulls, 10);
	if (ioctl(fn, TIOCCBRK, STBNULL) < 0) {
		DEBUG(5, "break TIOCCBRK %s\n", strerror(errno));
        }
# endif
	DEBUG(4, "ioctl %f second break\n", (float) bnulls/10 );
# else
	struct sgttyb ttbuf;
	register int sospeed;

	if (ioctl(fn, TIOCGETP, &ttbuf) < 0) {
		DEBUG(5, "break TIOCGETP %s\n", strerror(errno));
        }
	sospeed = ttbuf.sg_ospeed;
	ttbuf.sg_ospeed = BSPEED;
	if (ioctl(fn, TIOCSETP, &ttbuf) < 0) {
		DEBUG(5, "break TIOCSETP %s\n", strerror(errno));
        }
	if (write(fn, "\0\0\0\0\0\0\0\0\0\0\0\0", bnulls) != bnulls) {
badbreak:
		logent(strerror(errno), "BAD WRITE genbrk");
		alarm(0);
		longjmp(Sjbuf, 3);
	}
	ttbuf.sg_ospeed = sospeed;
	if (ioctl(fn, TIOCSETP, &ttbuf) < 0) {
		DEBUG(5, "break ioctl %s\n", strerror(errno));
        }
	if (write(fn, "@", 1) != 1)
		goto badbreak;
	DEBUG(4, "sent BREAK nulls - %d\n", bnulls);
#endif
#endif
}


#ifdef DIALINOUT
/* DIALIN/OUT CODE (WLS) */
/*
 * disable and reenable:  allow a single line to be use for dialin/dialout
 *
 */
char enbdev[16];

int disable(dev)
register char *dev;
{
	register char *rdev;

	/* strip off directory prefixes */
	rdev = dev;
	while (*rdev)
		rdev++;
	while (--rdev >= dev && *rdev != '/')
		;
	rdev++;

	if (enbdev[0]) {
		if (strcmp(enbdev, rdev) == SAME)
			return SUCCESS;	/* already disabled */
		delock(enbdev);
		reenable();		/* else, reenable the old one */
	}
	DEBUG(4, "Disable %s\n", rdev);
	if (enbcall("disable", rdev) == FAIL)
		return FAIL;
	strcpy(enbdev, rdev);
	return SUCCESS;
}

void reenable()
{
	if (enbdev[0] == '\0')
		return;
	DEBUG(4, "Reenable %s\n", enbdev);
	(void) enbcall("enable", enbdev);
	enbdev[0] = '\0';
}

int enbcall(type, dev)
char *type, *dev;
{
	int pid;
	register char *p;
	int fildes[2];
	int status;
	FILE *fil;
	char buf[80];

	fflush(stderr);
	fflush(stdout);
	pipe(fildes);
	if ((pid = fork()) == 0) {
		DEBUG(4, DIALINOUT, CNULL);
		DEBUG(4, " %s", type);
		DEBUG(4, " %s\n", dev);
		close(fildes[0]);
		close(0); close(1); close(2);
		open("/dev/null",0);
		dup(fildes[1]); dup(fildes[1]);
		setuid(geteuid());	/* for chown(uid()) in acu program */
		execl(DIALINOUT, "acu", type, dev, 0);
		exit(-1);
	}
	if (pid<0)
		return FAIL;

	close(fildes[1]);
	fil = fdopen(fildes[0],"r");
	if (fil!=NULL) {
#ifdef BSD4_2
		setlinebuf(fil);
#endif
		while (fgets(buf, sizeof buf, fil) != NULL) {
			p = buf + strlen(buf) - 1;
			if (*p == '\n')
				*p = '\0';
			logent(buf,"ACUCNTRL:");
		}
	}
	while(wait(&status) != pid)
		;
	fclose(fil);
	return status ? FAIL : SUCCESS;
}
#endif
