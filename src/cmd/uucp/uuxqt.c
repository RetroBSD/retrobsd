#include "uucp.h"
#include <sys/stat.h>
#include <strings.h>
#ifdef	NDIR
#include "ndir.h"
#else
#include <sys/dir.h>
#endif
#include <signal.h>
#include <sys/wait.h>

#define BADCHARS	"&^|(`\\<>;\"{}\n'"
#define RECHECKTIME	60*10	/* 10 minutes */

#define APPCMD(d) {\
char *p;\
for (p = d; *p != '\0';) *cmdp++ = *p++; *cmdp++ = ' '; *cmdp = '\0';}

/*
 *	uuxqt will execute commands set up by a uux command,
 *	usually from a remote machine - set by uucp.
 */

#define	NCMDS	50
char *Cmds[NCMDS+1];
int Notify[NCMDS+1];
#define	NT_YES	0	/* if should notify on execution */
#define	NT_ERR	1	/* if should notify if non-zero exit status (-z equivalent) */
#define	NT_NO	2	/* if should not notify ever (-n equivalent) */

extern int Nfiles;

int TransferSucceeded = 1;
int notiok = 1;
int nonzero = 0;

struct timeval Now;

char PATH[MAXFULLNAME] = "PATH=/bin:/usr/bin:/usr/ucb";
char Shell[MAXFULLNAME];
char HOME[MAXFULLNAME];

extern char **environ;
char *nenv[] = {
	PATH,
	Shell,
	HOME,
	0
};

static int gtxfile(char *file);
static int argok(char *xc, char *cmd);
static void notify(char *user, char *rmt, char *cmd, char *str);
static void mvxfiles(char *xfile);
static int shio(char *cmd, char *fi, char *fo);
static int chknotify(char *cmd);
static void rmxfiles(char *xfile);
static int gotfiles(char *file);

/*  to remove restrictions from uuxqt
 *  define ALLOK 1
 *
 *  to add allowable commands, add to the file CMDFILE
 *  A line of form "PATH=..." changes the search path
 */
int main(argc, argv)
char *argv[];
{
	char xcmd[MAXFULLNAME];
	int argnok;
	int notiflg;
	char xfile[MAXFULLNAME], user[MAXFULLNAME], buf[BUFSIZ];
	char lbuf[MAXFULLNAME];
	char cfile[NAMESIZE], dfile[MAXFULLNAME];
	char file[NAMESIZE];
	char fin[MAXFULLNAME], sysout[NAMESIZE], fout[MAXFULLNAME];
	register FILE *xfp, *fp;
	FILE *dfp;
	char path[MAXFULLNAME];
	char cmd[BUFSIZ];
	char *cmdp, prm[1000], *ptr;
	int uid, ret, ret2, badfiles;
	register int i;
	int stcico = 0;
	time_t xstart, xnow;
	char retstat[30];
	char **ep;

	strcpy(Progname, "uuxqt");
	uucpname(Myname);

	umask(WFMASK);
	Ofn = 1;
	Ifn = 0;
	while (argc>1 && argv[1][0] == '-') {
		switch(argv[1][1]){
		case 'x':
			chkdebug();
			Debug = atoi(&argv[1][2]);
			if (Debug <= 0)
				Debug = 1;
			break;
		default:
			fprintf(stderr, "unknown flag %s\n", argv[1]);
				break;
		}
		--argc;  argv++;
	}

	DEBUG(4, "\n\n** START **\n", CNULL);
	ret = subchdir(Spool);
	ASSERT(ret >= 0, "CHDIR FAILED", Spool, ret);
	strcpy(Wrkdir, Spool);
	uid = getuid();
	guinfo(uid, User, path);
	setgid(getegid());
	setuid(geteuid());

	DEBUG(4, "User - %s\n", User);
	if (ulockf(X_LOCK, X_LOCKTIME) != 0)
		exit(0);

	fp = fopen(CMDFILE, "r");
	if (fp == NULL) {
		logent(CANTOPEN, CMDFILE);
		Cmds[0] = "rmail";
		Cmds[1] = "rnews";
		Cmds[2] = "ruusend";
		Cmds[3] = NULL;
		goto doprocess;
	}
	DEBUG(5, "%s opened\n", CMDFILE);
	for (i=0; i<NCMDS && cfgets(xcmd, sizeof(xcmd), fp) != NULL; i++) {
		int j;
		/* strip trailing whitespace */
		for (j = strlen(xcmd)-1; j >= 0; --j)
			if (xcmd[j] == '\n' || xcmd[j] == ' ' || xcmd[j] == '\t')
				xcmd[j] = '\0';
			else
				break;
		/* look for imbedded whitespace */
		for (; j >= 0; --j)
			if (xcmd[j] == '\n' || xcmd[j] == ' ' || xcmd[j] == '\t')
				break;
		/* skip this entry if it has embedded whitespace */
		/* This defends against a bad PATH=, for example */
		if (j >= 0) {
			logent(xcmd, "BAD WHITESPACE");
			continue;
		}
		if (strncmp(xcmd, "PATH=", 5) == 0) {
			strcpy(PATH, xcmd);
			i--;	/*kludge */
			continue;
		}
		DEBUG(5, "xcmd = %s\n", xcmd);

		if ((ptr = index(xcmd, ',')) != NULL) {
			*ptr++ = '\0';
			if (strncmp(ptr, "Err", 3) == SAME)
				Notify[i] = NT_ERR;
			else if (strcmp(ptr, "No") == SAME)
				Notify[i] = NT_NO;
			else
				Notify[i] = NT_YES;
		} else
			Notify[i] = NT_YES;
		if ((Cmds[i] = (char *)malloc((unsigned)(strlen(xcmd)+1))) == NULL) {
			DEBUG(1, "MALLOC FAILED", CNULL);
			break;
		}
		strcpy(Cmds[i], xcmd);
	}
	Cmds[i] = CNULL;
	fclose(fp);

doprocess:

	(void) sprintf(HOME, "HOME=%s", Spool);
	(void) sprintf(Shell, "SHELL=%s", SHELL);
	environ = nenv; /* force use if our environment */

	DEBUG(11,"path = %s\n", getenv("PATH"));

	DEBUG(4, "process %s\n", CNULL);
	time(&xstart);
	while (gtxfile(xfile) > 0) {
		/* if /etc/nologin exists, exit cleanly */
#if defined(BSD4_2) || defined(USG)
		if (access(NOLOGIN) == 0) {
#else
		ultouch();
		if (nologinflag) {
#endif
			logent(NOLOGIN, "UUXQT SHUTDOWN");
			if (Debug)
				logent("debugging", "continuing anyway");
			else
				break;
		}
		DEBUG(4, "xfile - %s\n", xfile);

		xfp = fopen(subfile(xfile), "r");
		ASSERT(xfp != NULL, CANTOPEN, xfile, 0);

		/*  initialize to default  */
		strcpy(user, User);
		strcpy(fin, DEVNULL);
		strcpy(fout, DEVNULL);
		strcpy(sysout, Myname);
		badfiles = 0;
		while (fgets(buf, BUFSIZ, xfp) != NULL) {
			switch (buf[0]) {
			case X_USER:
				sscanf(&buf[1], "%s %s", user, Rmtname);
				break;
			case X_RETURNTO:
				sscanf(&buf[1], "%s", user);
				break;
			case X_STDIN:
				sscanf(&buf[1], "%s", fin);
				i = expfile(fin);
				/* rti!trt: do not check permissions of
				 * vanilla spool file */
				if (i != 0
				 && (chkpth("", "", fin) || anyread(fin) != 0))
					badfiles = 1;
				break;
			case X_STDOUT:
				sscanf(&buf[1], "%s%s", fout, sysout);
				sysout[MAXBASENAME] = '\0';
				/* rti!trt: do not check permissions of
				 * vanilla spool file.  DO check permissions
				 * of writing on a non-vanilla file */
				i = 1;
				if (fout[0] != '~' || prefix(sysout, Myname))
					i = expfile(fout);
				if (i != 0
				 && (chkpth("", "", fout)
					|| chkperm(fout, (char *)1)))
					badfiles = 1;
				break;
			case X_CMD:
				strcpy(cmd, &buf[2]);
				if (*(cmd + strlen(cmd) - 1) == '\n')
					*(cmd + strlen(cmd) - 1) = '\0';
				break;
			case X_NONOTI:
				notiok = 0;
				break;
			case X_NONZERO:
				nonzero = 1;
				break;
			default:
				break;
			}
		}

		fclose(xfp);
		DEBUG(4, "fin - %s, ", fin);
		DEBUG(4, "fout - %s, ", fout);
		DEBUG(4, "sysout - %s, ", sysout);
		DEBUG(4, "user - %s\n", user);
		DEBUG(4, "cmd - %s\n", cmd);

		/*  command execution  */
		if (strcmp(fout, DEVNULL) == SAME)
			strcpy(dfile,DEVNULL);
		else
			gename(DATAPRE, sysout, 'O', dfile);

		/* expand file names where necessary */
		expfile(dfile);
		cmdp = buf;
		ptr = cmd;
		xcmd[0] = '\0';
		argnok = 0;
		while ((ptr = getprm(ptr, prm)) != NULL) {
			if (prm[0] == ';' || prm[0] == '^'
			  || prm[0] == '&'  || prm[0] == '|') {
				xcmd[0] = '\0';
				APPCMD(prm);
				continue;
			}

			if ((argnok = argok(xcmd, prm)) != SUCCESS)
				/*  command not valid  */
				break;

			if (prm[0] == '~')
				expfile(prm);
			APPCMD(prm);
		}
		/*
		 * clean up trailing ' ' in command.
		 */
		if (cmdp > buf && cmdp[0] == '\0' && cmdp[-1] == ' ')
			*--cmdp = '\0';
		if (argnok || badfiles) {
			sprintf(lbuf, "%s XQT DENIED", user);
			logent(cmd, lbuf);
			DEBUG(4, "bad command %s\n", prm);
			notify(user, Rmtname, cmd, "DENIED");
			goto rmfiles;
		}
		sprintf(lbuf, "%s XQT", user);
		logent(buf, lbuf);
		DEBUG(4, "cmd %s\n", buf);

		mvxfiles(xfile);
		ret = subchdir(XQTDIR);
		ASSERT(ret >= 0, "CHDIR FAILED", XQTDIR, ret);
		ret = shio(buf, fin, dfile);
		sprintf(retstat, "signal %d, exit %d", ret & 0377,
		  (ret>>8) & 0377);
		if (strcmp(xcmd, "rmail") == SAME)
			notiok = 0;
		if (strcmp(xcmd, "rnews") == SAME)
			nonzero = 1;
		notiflg = chknotify(xcmd);
		if (notiok && notiflg != NT_NO &&
		   (ret != 0 || (!nonzero && notiflg == NT_YES)))
			notify(user, Rmtname, cmd, retstat);
		else if (ret != 0 && strcmp(xcmd, "rmail") == SAME) {
			/* mail failed - return letter to sender  */
#ifdef	DANGEROUS
			/* NOT GUARANTEED SAFE!!! */
			if (!nonzero)
				retosndr(user, Rmtname, fin);
#else
			notify(user, Rmtname, cmd, retstat);
#endif
			sprintf(buf, "%s (%s) from %s!%s", buf, retstat, Rmtname, user);
			logent("MAIL FAIL", buf);
		}
		DEBUG(4, "exit cmd - %d\n", ret);
		ret2 = subchdir(Spool);
		ASSERT(ret2 >= 0, "CHDIR FAILED", Spool, ret);
		rmxfiles(xfile);
		if (ret != 0) {
			/*  exit status not zero */
			dfp = fopen(subfile(dfile), "a");
			ASSERT(dfp != NULL, CANTOPEN, dfile, 0);
			fprintf(dfp, "exit status %d", ret);
			fclose(dfp);
		}
		if (strcmp(fout, DEVNULL) != SAME) {
			if (prefix(sysout, Myname)) {
				xmv(dfile, fout);
				chmod(fout, BASEMODE);
			} else {
				char *cp = rindex(user, '!');
				gename(CMDPRE, sysout, 'O', cfile);
				fp = fopen(subfile(cfile), "w");
				ASSERT(fp != NULL, "OPEN", cfile, 0);
				fprintf(fp, "S %s %s %s - %s 0666\n", dfile,
					fout, cp ? cp : user, lastpart(dfile));
				fclose(fp);
			}
		}
	rmfiles:
		xfp = fopen(subfile(xfile), "r");
		ASSERT(xfp != NULL, CANTOPEN, xfile, 0);
		while (fgets(buf, BUFSIZ, xfp) != NULL) {
			if (buf[0] != X_RQDFILE)
				continue;
			sscanf(&buf[1], "%s", file);
			unlink(subfile(file));
		}
		unlink(subfile(xfile));
		fclose(xfp);

		/* rescan X. for new work every RECHECKTIME seconds */
		time(&xnow);
		if (xnow > (xstart + RECHECKTIME)) {
			extern int Nfiles;
			Nfiles = 0; 	/*force rescan for new work */
		}
		xstart = xnow;
	}

	if (stcico)
		xuucico("");
	cleanup(0);
}

void cleanup(code)
int code;
{
	logcls();
	rmlock(CNULL);
#ifdef	VMS
	/*
	 *	Since we run as a BATCH job we must wait for all processes to
	 *	to finish
	 */
	while(wait(0) != -1)
		;
#endif
	exit(code);
}

/*
 *	get a file to execute
 *
 *	return codes:  0 - no file  |  1 - file to execute
 */
int gtxfile(file)
register char *file;
{
	char pre[3];
	int rechecked;
	time_t ystrdy;		/* yesterday */
	struct stat stbuf;	/* for X file age */

	pre[0] = XQTPRE;
	pre[1] = '.';
	pre[2] = '\0';
	rechecked = 0;
retry:
	if (!gtwrkf(Spool, file)) {
		if (rechecked)
			return 0;
		rechecked = 1;
		DEBUG(4, "iswrk\n", CNULL);
		if (!iswrk(file, "get", Spool, pre))
			return 0;
	}
	DEBUG(4, "file - %s\n", file);
	/* skip spurious subdirectories */
	if (strcmp(pre, file) == SAME)
		goto retry;
	if (gotfiles(file))
		return 1;
	/* check for old X. file with no work files and remove them. */
	if (Nfiles > LLEN/2) {
	    time(&ystrdy);
	    ystrdy -= (4 * 3600L);		/* 4 hours ago */
	    DEBUG(4, "gtxfile: Nfiles > LLEN/2\n", CNULL);
	    while (gtwrkf(Spool, file) && !gotfiles(file)) {
		if (stat(subfile(file), &stbuf) == 0)
		    if (stbuf.st_mtime <= ystrdy) {
			char *bnp, cfilename[NAMESIZE];
			DEBUG(4, "gtxfile: move %s to CORRUPT \n", file);
			unlink(subfile(file));
			bnp = rindex(subfile(file), '/');
			sprintf(cfilename, "%s/%s", CORRUPT,
				bnp ? bnp + 1 : subfile(file));
			xmv(subfile(file), cfilename);
			logent(file, "X. FILE CORRUPTED");
		    }
	    }
	    DEBUG(4, "iswrk\n", CNULL);
	    if (!iswrk(file, "get", Spool, pre))
		return 0;
	}
	goto retry;
}

/*
 *	check for needed files
 *
 *	return codes:  0 - not ready  |  1 - all files ready
 */
int gotfiles(file)
register char *file;
{
	struct stat stbuf;
	register FILE *fp;
	char buf[BUFSIZ], rqfile[MAXFULLNAME];

	fp = fopen(subfile(file), "r");
	if (fp == NULL)
		return 0;

	while (fgets(buf, BUFSIZ, fp) != NULL) {
		DEBUG(4, "%s\n", buf);
		if (buf[0] != X_RQDFILE)
			continue;
		sscanf(&buf[1], "%s", rqfile);
		expfile(rqfile);
		if (stat(subfile(rqfile), &stbuf) == -1) {
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);
	return 1;
}

/*
 *	remove execute files to x-directory
 */
void rmxfiles(xfile)
register char *xfile;
{
	register FILE *fp;
	char buf[BUFSIZ], file[NAMESIZE], tfile[NAMESIZE];
	char tfull[MAXFULLNAME];

	if((fp = fopen(subfile(xfile), "r")) == NULL)
		return;

	while (fgets(buf, BUFSIZ, fp) != NULL) {
		if (buf[0] != X_RQDFILE)
			continue;
		if (sscanf(&buf[1], "%s%s", file, tfile) < 2)
			continue;
		sprintf(tfull, "%s/%s", XQTDIR, tfile);
		unlink(subfile(tfull));
	}
	fclose(fp);
	return;
}

/*
 *	move execute files to x-directory
 */
void mvxfiles(xfile)
char *xfile;
{
	register FILE *fp;
	char buf[BUFSIZ], ffile[MAXFULLNAME], tfile[NAMESIZE];
	char tfull[MAXFULLNAME];
	int ret;

	if((fp = fopen(subfile(xfile), "r")) == NULL)
		return;

	while (fgets(buf, BUFSIZ, fp) != NULL) {
		if (buf[0] != X_RQDFILE)
			continue;
		if (sscanf(&buf[1], "%s%s", ffile, tfile) < 2)
			continue;
		expfile(ffile);
		sprintf(tfull, "%s/%s", XQTDIR, tfile);
		unlink(subfile(tfull));
		ret = xmv(ffile, tfull);
		ASSERT(ret == 0, "XQTDIR ERROR", CNULL, ret);
	}
	fclose(fp);
}

/*
 *	check for valid command/argument
 *	*NOTE - side effect is to set xc to the	command to be executed.
 *
 *	return 0 - ok | 1 nok
 */
int argok(xc, cmd)
register char *xc, *cmd;
{
	register char **ptr;

#ifndef ALLOK
	if (strpbrk(cmd, BADCHARS) != NULL) {
		DEBUG(1,"MAGIC CHARACTER FOUND\n", CNULL);
		logent(cmd, "NASTY MAGIC CHARACTER FOUND");
		return FAIL;
	}
#endif

	if (xc[0] != '\0')
		return SUCCESS;

#ifndef ALLOK
	ptr = Cmds;
	DEBUG(9, "Compare %s and\n", cmd);
	while(*ptr != NULL) {
		DEBUG(9, "\t%s\n", *ptr);
		if (strcmp(cmd, *ptr) == SAME)
			break;
		ptr++;
	}
	if (*ptr == NULL) {
		DEBUG(1,"COMMAND NOT FOUND\n", CNULL);
		return FAIL;
	}
#endif
	strcpy(xc, cmd);
	DEBUG(9, "MATCHED %s\n", xc);
	return SUCCESS;
}

/*
 *	if notification should be sent for successful execution of cmd
 *
 *	return NT_YES - do notification
 *	       NT_ERR - do notification if exit status != 0
 *	       NT_NO  - don't do notification ever
 */
int chknotify(cmd)
char *cmd;
{
	register char **ptr;
	register int *nptr;

	ptr = Cmds;
	nptr = Notify;
	while (*ptr != NULL) {
		if (strcmp(cmd, *ptr) == SAME)
			return *nptr;
		ptr++;
		nptr++;
	}
	return NT_YES;		/* "shouldn't happen" */
}

/*
 *	send mail to user giving execution results
 */
void notify(user, rmt, cmd, str)
char *user, *rmt, *cmd, *str;
{
	char text[MAXFULLNAME];
	char ruser[MAXFULLNAME];

	if (strpbrk(user, BADCHARS) != NULL) {
		char lbuf[MAXFULLNAME];
		sprintf(lbuf, "%s INVALID CHARACTER IN USERNAME", user);
		logent(cmd, lbuf);
		strcpy(user, "postmaster");
	}
	sprintf(text, "uuxqt cmd (%s) status (%s)", cmd, str);
	if (prefix(rmt, Myname))
		strcpy(ruser, user);
	else
		sprintf(ruser, "%s!%s", rmt, user);
	mailst(ruser, text, CNULL);
}

/*
 *	return mail to sender
 *
 */
void retosndr(user, rmt, file)
char *user, *rmt, *file;
{
	char ruser[MAXFULLNAME];

	if (strpbrk(user, BADCHARS) != NULL) {
		char lbuf[MAXFULLNAME];
		sprintf(lbuf, "%s INVALID CHARACTER IN USERNAME", user);
		logent(file, lbuf);
		strcpy(user, "postmaster");
	}
	if (strcmp(rmt, Myname) == SAME)
		strcpy(ruser, user);
	else
		sprintf(ruser, "%s!%s", rmt, user);

	if (anyread(file) == 0)
		mailst(ruser, "Mail failed.  Letter returned to sender.\n", file);
	else
		mailst(ruser, "Mail failed.  Letter returned to sender.\n", CNULL);
	return;
}

/*
 *	execute shell of command with fi and fo as standard input/output
 */
int shio(cmd, fi, fo)
char *cmd, *fi, *fo;
{
	int status, f;
	int uid, pid, ret;
	char path[MAXFULLNAME];
	char *args[20];
	extern int errno;

	if (fi == NULL)
		fi = DEVNULL;
	if (fo == NULL)
		fo = DEVNULL;

	getargs(cmd, args, 20);
	DEBUG(3, "shio - %s\n", cmd);
#ifdef SIGCHLD
	signal(SIGCHLD, SIG_IGN);
#endif
	if ((pid = fork()) == 0) {
		signal(SIGINT, SIG_IGN);
		signal(SIGHUP, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		close(Ifn);
		close(Ofn);
		close(0);
		setuid(getuid());
		f = open(subfile(fi), 0);
		if (f != 0) {
			logent(fi, "CAN'T READ");
			exit(-errno);
		}
		close(1);
		f = creat(subfile(fo), 0666);
		if (f != 1) {
			logent(fo, "CAN'T WRITE");
			exit(-errno);
		}
		execvp(args[0], args);
		exit(100+errno);
	}
	while ((ret = wait(&status)) != pid && ret != -1)
		;
	DEBUG(3, "status %d\n", status);
	return status;
}
