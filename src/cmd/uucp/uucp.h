/*	uucp.h	5.11.6	1997/10/2 */

#include <stdio.h>
#include <paths.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef	NDIR
#include "ndir.h"
#else
#include <sys/dir.h>
#endif

/*
 * Determine local uucp name of this machine.
 * Define one of the following:
 *
 * For UCB 4.1A and later systems, you will have the gethostname(2) call.
 * If this call exists, define GETHOSTNAME.
 *
 * For USG 3.0 and later systems, you will have the uname(2) call.
 * If this call exists, define UNAME.
 *
 * Some systems have a line of the form '#define sysname "myuucpname",'
 * in the file /usr/include/whoami.h, to identify their machine.
 * If your site does that, define WHOAMI.
 *
 * If your site has <whoami.h>, but you do not want to read
 * that file every time uucp runs, you can compile sysname into uucp.
 * This is faster and more reliable, but binaries do not port.
 * If you want to do that, define CCWHOAMI.
 *
 * Some systems put the local uucp name in a single-line file
 * named /etc/uucpname or /local/uucpname.
 * If your site does that, define UUNAME.
 *
 * Systems running 3Com's UNET will have the getmyhname() call.
 * If you want to, define GETMYHNAME.
 *
 * You should also define MYNAME to be your uucp name.
 *
 * For each of the above that are defined, uucp checks them in order.
 * It stops on the first method that returns a non null name.
 * If everything fails, it uses "unknown" for the system name.
 */
#define	GETHOSTNAME	/**/
// #define UNAME	/**/
// #define WHOAMI	/**/
// #define CCWHOAMI	/**/
// #define UUNAME	/**/
// #define GETMYHNAME	/**/
/* If the above fails ... */
#define	MYNAME	"wlonex"

/*
 * If you have it, include <sysexits.h> to use exit
 * codes that will be understood by sendmail.
 * Otherwise, define EX_NOHOST, EX_CANTCREAT, and EX_NOINPUT.
 */
#include <sysexits.h>
//#define EX_NOINPUT	66	/**/
//#define EX_NOHOST	68	/**/
//#define EX_CANTCREAT	73	/**/

/*
 * Define the various kinds of connections to include.
 * The complete list is in the condevs array in condevs.c
 */
// #define ATT2224		/* AT&T 2224 */
//#define BSDTCP		/* 4.2bsd or 2.9bsd TCP/IP */
// #define CDS224		/* Concord Data Systems 2400 */
// #define DATAKIT	/* ATT's datakit */
// #define DF02		/* Dec's DF02/DF03 */
// #define DF112		/* Dec's DF112 */
// #define DN11		/* "standard" DEC dialer */
#define HAYES		/* Hayes' Smartmodem */
// #define HAYES2400	/* Hayes' 2400 baud Smartmodem */
// #define MICOM	/* Micom Mux port */
// #define NOVATION	/* Novation modem */
// #define PAD		/* X.25 PAD */
// #define PENRIL		/* PENRIL Dialer */
// #define PNET		/* Purdue network */
// #define RVMACS		/* Racal-Vadic MACS  820 dialer, 831 adaptor */
// #define SYTEK	/* Sytek Local Area Net */
// #define UNETTCP	/* 3Com's UNET */
// #define USR2400		/* USRobotics Courier 2400 */
// #define VA212		/* Racal-Vadic 212 */
// #define VA811S		/* Racal-Vadic 811S dialer, 831 adaptor */
// #define VA820		/* Racal-Vadic 820 dialer, 831 adaptor */
// #define VADIC		/* Racal-Vadic 345x */
// #define VENTEL		/* Ventel Dialer */
// #define VMACS		/* Racal-Vadic MACS  811 dialer, 831 adaptor */

#if defined(USR2400) && !defined(HAYES)
#define HAYES
#endif

#if defined(UNETTCP) || defined(BSDTCP)
#define TCPIP
#endif

/*
 * We need a timer to write slowly to certain modems.
 * and for generating breaks.
 *
 * define INTERVALTIMER to use 4.[23] bsd interval timer.
 * define FASTTIMER if you have the nap() system call.
 * define FTIME if you have the ftime() system call.
 * define BUSYLOOP if you must do a busy loop.
 * Look at uucpdelay() in condevs.c for details.
 */
#define INTERVALTIMER /**/
//#define FASTTIMER /**/
//#define FTIME /**/
//#define BUSYLOOP /**/

/*
 * If your site is using "ndir.h" to retrofit the Berkeley
 * directory reading routines, define NDIR.
 * You will probably also have to set LIBNDIR in Makefile.
 * Otherwise, <dir.h> is assumed to have the Berkeley directory definitions.
 */
//#define	NDIR	/**/

/*
 * If yours is a BTL system III, IV, V or so-on site, define USG.
 */
//#define	USG	/**/

/*
 * If you are running 4.3bsd, define BSD4_3 and BSD4_2
 * If you are just running 4.2bsd, define BSD4_2
 * If you are running 2.11bsd, define BSD4_3 and BSD4_2
 * If you are running the BRL version of 4.2BSD define BRL4_2, NOT BSD4_3
 */
#define BSD4_3 	/**/
#define BSD4_2 	/**/
//#define BRL4_2 /**/

#if defined(BRL4_2) && !defined(BSD4_2)
#define BSD4_2
#undef BSD4_3
#endif

/*
 * If you are running 2.9bsd define BSD2_9
 */
//#define BSD2_9 	/**/

/*
 * If you are using 'inetd' with 4.2bsd, define BSDINETD
 */
// #define BSDINETD	/**/

/*
 * If you are running 4.3bsd, 2.11bsd or BRL 4.2, you are running 'inetd'
 */

#if (defined(BSD4_3) || defined(BRL4_2)) && !defined(BSDINETD)
#define BSDINETD
#endif

//#define VMSDTR
                        /* Turn on modem control on vms(works DTR) for
			   develcon and gandalf ports to gain access */
/*
 *	If you want to use the same modem for dialing in and out define
 *	DIALINOUT to be the localtion of the acucntrl program
 */
// #define DIALINOUT	"/usr/libexec/acucntrl" /**/

/*
 *	If you want all ACU lines to be DIALINOUT, define ALLACUINOUT
 */
// #define ALLACUINOUT	/**/

/* define the value of WFMASK - for umask call - used for all uucp work files */
#define WFMASK 0137

/* define the value of LOGMASK - for LOGFILE, SYSLOG, ERRLOG */
#define	LOGMASK		0133

/* All files are given at least the following at the final destination */
/* It is also the default mode, so '666' is recommended */
/* and 444 is minimal (minimally useful, maximally annoying) */
#define	BASEMODE	0666

/*
 * Define NOSTRANGERS if you don't want to accept transactions from
 * sites that are not in your L.sys file (see cico.c)
 */
#define NOSTRANGERS	/**/

/*
 * Traditionally LCK (lock) files have been kept in /usr/spool/uucp.
 * If you want that define LOCKDIR to be ".".
 * If you want the locks kept in a subdirectory, define LOCKDIR as "LCK".
 * Good news about LCK. subdirectory: the directory can be mode 777 so
 * unprivileged programs can share the uucp locking system,
 * and the subdirectory keeps down clutter in the main directory.
 * The BAD news: you have to change 'tip' and another programs that
 * know where the LCK files are kept, and you have to change your /etc/rc
 * if your rc cleans out the lock files (as it should).
 */
//#define	LOCKDIR	"LCK"	/**/
#define LOCKDIR	"." /**/

/*
 * If you want uucp and uux to copy the data files by default,
 * don't define DONTCOPY (This is the way older 4bsd uucps worked)
 * If you want uucp and uux to use the original files instead of
 * copies, define DONTCOPY (This is the way System III and V work)
 */
#define DONTCOPY	/**/

/*
 * Very few (that I know of) systems use the sequence checking feature.
 * If you are not going to use it (hint: you are not),
 * do not define GNXSEQ.  This saves precious room on PDP11s.
 */
//#define	GNXSEQ	/* comment this out to save space */

/*
 * If you want the logfile stored in a file for each site instead
 * of one file
 * define LOGBYSITE as the directory to put the files in
 */
//#define LOGBYSITE	"/usr/spool/uucp/LOG" /**/

#define XQTDIR		"/usr/spool/uucp/XTMP"
#define SQFILE		"/etc/uucp/SQFILE"
#define SQTMP		"/etc/uucp/SQTMP"
#define SLCKTIME	5400	/* system/device timeout (LCK.. files) */
#define SEQFILE		"/etc/uucp/SEQF"
#define SYSFILE		"/etc/uucp/L.sys"
#define DEVFILE		"/etc/uucp/L-devices"
#define DIALFILE	"/etc/uucp/L-dialcodes"
#define USERFILE	"/etc/uucp/USERFILE"
#define	CMDFILE		"/etc/uucp/L.cmds"
#define	ALIASFILE	"/etc/uucp/L.aliases"

#define SPOOL		"/usr/spool/uucp"
#define SYSLOG		"/usr/spool/uucp/SYSLOG"
#define PUBDIR		"/usr/spool/uucppublic"

#define SQLOCK		"SQ"
#define SEQLOCK		"SEQL"
#define CMDPRE		'C'
#define DATAPRE		'D'
#define XQTPRE		'X'

#define LOGFILE		"/usr/spool/uucp/LOGFILE"
#define ERRLOG		"/usr/spool/uucp/ERRLOG"
#define CMDSDIR		"/usr/spool/uucp/C."
#define DATADIR		"/usr/spool/uucp/D."
#define XEQTDIR		"/usr/spool/uucp/X."

#define RMTDEBUG	"AUDIT"
#define CORRUPT		"CORRUPT"
#define SQTIME		60
#define TRYCALLS	2	/* number of tries to dial call */

#define LLEN	50
#define MAXRQST	250

#define DEBUG(l, f, s) if (Debug >= l) fprintf(stderr, f, s); else

#define ASSERT(e, s1, s2, i1) if (!(e)) {assert(s1, s2, i1);cleanup(FAIL);}else

#define delock(dev)	rmlock(dev)
#define mlock(dev)	ulockf(dev, SLCKTIME)

#define SAME		0
#define ANYREAD		0004
#define ANYWRITE	02
#define FAIL		-1
#define SUCCESS		0
#define CNULL		(char *) 0
#define STBNULL		(struct sgttyb *) 0
#define MASTER		1
#define SLAVE		0
#define MAXFULLNAME	255
#define MAXMSGTIME	45
#define NAMESIZE	255
#define MAXBASENAME	14
#define SYSNSIZE	(MAXBASENAME-1-1-1-4)
#define EOTMSG		"\04\n\04\n"
#define CALLBACK	1
#define ONEDAY		86400L

	/*  commands  */
#define SHELL		"/bin/sh"
#define UUCICO		"/usr/sbin/uucico"
#define UUXQT		"/usr/libexec/uuxqt"
#define UUCP		"uucp"

	/*  call connect fail stuff  */
#define CF_SYSTEM	-1
#define CF_TIME		-2
#define CF_LOCK		-3
#define	CF_NODEV	-4
#define CF_DIAL		-5
#define CF_LOGIN	-6

#define F_NAME		0
#define F_TIME		1
#define F_LINE		2
#define F_CLASS		3	/* an optional prefix and the speed */
#define F_PHONE		4
#define F_LOGIN		5

#define MAXPH		60	/* maximum length of a phone number */

	/* This structure tells about a device */
struct Devices {
#define	D_type		D_arg[0]
#define	D_line		D_arg[1]
#define	D_calldev	D_arg[2]
#define	D_class		D_arg[3]
#define	D_brand		D_arg[4]
#define	D_CHAT		5
	int  D_numargs;
	int  D_speed;
	char *D_arg[20];
	char D_argbfr[100];
};

	/* This structure tells how to get to a device */
struct condev {
	char *CU_meth;			/* method, such as 'ACU' or 'DIR' */
	char *CU_brand;			/* brand, such as 'Hayes' or 'Vadic' */
	int (*CU_gen)(char *flds[]);	/* what to call to search for brands */
	int (*CU_open)(char *telno, char *flds[], struct Devices *dev);
					/* what to call to open brand */
	int (*CU_clos)(int fd);		/* what to call to close brand */
};

	/*  system status stuff  */
#define SS_OK		0
#define SS_NODEVICE	1
#define SS_CALLBACK	2
#define SS_INPROGRESS	3
#define SS_FAIL		4
#define SS_BADSEQ	5
#define SS_WRONGTIME	6

	/*  fail/retry parameters  */
#define RETRYTIME	600
#define MAXRECALLS	25

	/*  stuff for command execution  */
#define X_RQDFILE	'F'
#define X_STDIN		'I'
#define X_STDOUT	'O'
#define X_CMD		'C'
#define X_USER		'U'
#define X_SENDFILE	'S'
#define	X_NONOTI	'N'
#define X_RETURNTO	'R'
#define	X_NONZERO	'Z'
#define X_LOCK		"XQT"
#define X_LOCKTIME	3600L

#define WKDSIZE		100	/*  size of work dir name  */

#include <sys/types.h>

#define rindex strrchr
#define index strchr

extern struct timeval Now;

extern int Ifn, Ofn;
extern char *Rmtname;
extern char User[];
extern char Loginuser[];
extern char *Spool;
extern char Myname[];
extern char Myfullname[];
extern int Debug;
extern int Bspeed;
extern char Wrkdir[];
extern time_t Retrytime;
extern short Usrf;
extern int IsTcpIp;
extern char Progname[];
extern int (*CU_end)();
extern struct condev condevs[];
extern char NOLOGIN[];

extern	char DLocal[], DLocalX[];

extern char _FAILED[], CANTOPEN[], DEVNULL[];

#ifdef lint
/* This horrible gross kludge is the only way I know to
 * convince lint that signal(SIGINT,SIG_IGN) is legal. It hates SIG_IGN.
 */
#ifdef SIG_IGN
#undef SIG_IGN
#endif
#define SIG_IGN	main
extern int main();
#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG(a,b,c)
#endif

void logent(char *text, char *status);
void getnextfd(void);
int fixline(int tty, int spwant);
int dochat(struct Devices *dev, char *flds[], int fd);
int expect(char *str, int fn);
void rmlock(char *name);
int guinfo(int uid, char *name, char *path);
void uucpname(char *name);
void assert(char *s1, char *s2, int i1);
void chkdebug(void);
int subchdir(char *s);
void fixmode(int tty);
int omsg(char type, char *msg, int fn);
int imsg(char *amsg, int fn);
int versys(char **nameptr);
int ulockf(char *hfile, time_t atime);
int callback(char *name);
void systat(char *name, int type, char *text);
void gename(char pre, char *sys, char grade, char *file);
char *subfile(char *as);
void xuucico(char *rmtname);
int callok(char *name);
void clsacu(void);
int gnsys(char *sname, char *dir, char pre);
int conn(char *system);
int startup(int role);
int cntrl(int role, char *wkpre);
void rmstat(char *name);
void logcls(void);
void xuuxqt(void);
void fioclex(int fd);
void cleanup(int code);
int gninfo(char *name, int *uid, char *path);
int prefix(char *s1, char *s2);
int wprefix(char *s1, char *s2);
char *cfgets(char *buf, int siz, FILE *fil);
int getargs(char *s, char *arps[], int maxargs);
int mkdirs(char *name);
int xmv(char *f1, char *f2);
char *subdir(char *d, char pre);
int gnamef(DIR *dirp, char *filename);
int iswrk(char *file, char *reqst, char *dir, char *pre);
int gturnon(void);
int gturnoff(void);
int grdmsg(char *str, int fn);
int gwrmsg(char type, char *str, int fn);
int grddata(int fn, FILE *fp);
int gwrdata(FILE *fp, int fn);
void setupline(char type);
int gtwvec(char *file, char *dir, char *wkpre, char **wrkvec);
int expfile(char *file);
int chkpth(char *logname, char *mchname, char *path);
int chkperm(char *file, char *mopt);
void xuucp(char *str);
int isdir(char *name);
char *lastpart(char *file);
int anyread(char *file);
void mailst(char *user, char *str, char *file);
void sysacct(long bytes, time_t time);
void syslog(char *text);
int chksum(char *s, int n);
int snccmp(char *s1, char *s2);
int rddev(FILE *fp, struct Devices *dev);
void alarmtr(int sig);
int nulldev(void);
int diropn(char *flds[]);
void genbrk(int fn, int bnulls);
void exphone(char *in, char *out);
int ckexpf(char *file);
int xcp(char *f1, char *f2);
char *getprm(char *s, char *prm);
int gtwrkf(char *dir, char *file);
