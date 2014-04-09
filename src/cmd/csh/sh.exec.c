/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	!defined(lint) && defined(DOSCCS)
static char *sccsid = "@(#)sh.exec.c	5.2.2 (2.11BSD) 1996/9/20";
#endif

#include "sh.h"
#include <string.h>
#include <sys/dir.h>

/*
 * C shell
 */

/*
 * System level search and execute of a command.
 * We look in each directory for the specified command name.
 * If the name contains a '/' then we execute only the full path name.
 * If there is no search path then we execute only full path names.
 */

/*
 * As we search for the command we note the first non-trivial error
 * message for presentation to the user.  This allows us often
 * to show that a file has the wrong mode/no access when the file
 * is not in the last component of the search path, so we must
 * go on after first detecting the error.
 */
const char *exerr;		/* Execution error message */
char	*expath;		/* Path for exerr */

#include "sh.exec.h"

/* Dummy search path for just absolute search when no path */
char	*justabs[] =	{ "", 0 };

doexec(t)
	register struct command *t;
{
	char *sav;
	register char *dp, **pv, **av;
	register struct varent *v;
	bool slash = any('/', t->t_dcom[0]);
	int hashval, hashval1, i;
	char *blk[2];

	/*
	 * Glob the command name.  If this does anything, then we
	 * will execute the command only relative to ".".  One special
	 * case: if there is no PATH, then we execute only commands
	 * which start with '/'.
	 */
	dp = globone(t->t_dcom[0]);
	sav = t->t_dcom[0];
	exerr = 0;
        expath = t->t_dcom[0] = dp;
	xfree(sav);
	v = adrof("path");
	if (v == 0 && expath[0] != '/')
		pexerr();
	slash |= gflag;

	/*
	 * Glob the argument list, if necessary.
	 * Otherwise trim off the quote bits.
	 */
	gflag = 0; av = &t->t_dcom[1];
	tglob(av);
	if (gflag) {
		av = glob(av);
		if (av == 0)
			error("No match");
	}
	blk[0] = t->t_dcom[0];
	blk[1] = 0;
	av = blkspl(blk, av);
#ifdef VFORK
	Vav = av;
#endif
	trim(av);

	xechoit(av);		/* Echo command if -x */
	/*
	 * Since all internal file descriptors are set to close on exec,
	 * we don't need to close them explicitly here.  Just reorient
	 * ourselves for error messages.
	 */
	SHIN = 0; SHOUT = 1; SHDIAG = 2; OLDSTD = 0;

	/*
	 * We must do this AFTER any possible forking (like `foo`
	 * in glob) so that this shell can still do subprocesses.
	 */
	(void) sigsetmask(0L);

	/*
	 * If no path, no words in path, or a / in the filename
	 * then restrict the command search.
	 */
	if (v == 0 || v->vec[0] == 0 || slash)
		pv = justabs;
	else
		pv = v->vec;
	sav = strspl("/", *av);		/* / command name for postpending */
#ifdef VFORK
	Vsav = sav;
#endif
	if (havhash)
		hashval = hashname(*av);
	i = 0;
#ifdef VFORK
	hits++;
#endif
	do {
		if (!slash && pv[0][0] == '/' && havhash) {
			hashval1 = hash(hashval, i);
			if (!bit(xhash, hashval1))
				goto cont;
		}
		if (pv[0][0] == 0 || eq(pv[0], "."))	/* don't make ./xxx */
			texec(*av, av);
		else {
			dp = strspl(*pv, sav);
#ifdef VFORK
			Vdp = dp;
#endif
			texec(dp, av);
#ifdef VFORK
			Vdp = 0;
#endif
			xfree(dp);
		}
#ifdef VFORK
		misses++;
#endif
cont:
		pv++;
		i++;
	} while (*pv);
#ifdef VFORK
	hits--;
#endif
#ifdef VFORK
	Vsav = 0;
	Vav = 0;
#endif
	xfree(sav);
	xfree((char *)av);
	pexerr();
}

pexerr()
{
	/* Couldn't find the damn thing */
	setname(expath);
	/* xfree(expath); */
	if (exerr)
		bferr(exerr);
	bferr("Command not found");
}

/*
 * Execute command f, arg list t.
 * Record error message if not found.
 * Also do shell scripts here.
 */
texec(f, t)
	char *f;
	register char **t;
{
	register struct varent *v;
	register char **vp;
	char *lastsh[2];

	execv(f, t);
	switch (errno) {

	case ENOEXEC:
		/*
		 * If there is an alias for shell, then
		 * put the words of the alias in front of the
		 * argument list replacing the command name.
		 * Note no interpretation of the words at this point.
		 */
		v = adrof1("shell", &aliases);
		if (v == 0) {
#ifdef OTHERSH
			register int ff = open(f, 0);
			char ch;
#endif

			vp = lastsh;
			vp[0] = adrof("shell") ? value("shell") : SHELLPATH;
			vp[1] = (char *) NULL;
#ifdef OTHERSH
			if (ff != -1 && read(ff, &ch, 1) == 1 && ch != '#')
				vp[0] = OTHERSH;
			(void) close(ff);
#endif
		} else
			vp = v->vec;
		t[0] = f;
		t = blkspl(vp, t);		/* Splice up the new arglst */
		f = *t;
		xfree((char *)t);
		execv(f, t);
		/* The sky is falling, the sky is falling! */

	case ENOMEM:
		Perror(f);

	case ENOENT:
		break;

	default:
		if (exerr == 0) {
			exerr = strerror(errno);
			expath = savestr(f);
		}
	}
}

/*ARGSUSED*/
execash(t, kp)
	char **t;
	register struct command *kp;
{

	rechist();
	(void) signal(SIGINT, parintr);
	(void) signal(SIGQUIT, parintr);
	(void) signal(SIGTERM, parterm);	/* if doexec loses, screw */
	lshift(kp->t_dcom, 1);
	exiterr++;
	doexec(kp);
	/*NOTREACHED*/
}

xechoit(t)
	char **t;
{

	if (adrof("echo")) {
		flush();
		haderr = 1;
		blkpr(t), putchar('\n');
		haderr = 0;
	}
}

/*VARARGS0*//*ARGSUSED*/
int dohash()
{
	struct stat stb;
	DIR *dirp;
	register struct direct *dp;
	register int cnt;
	int i = 0;
	struct varent *v = adrof("path");
	char **pv;
	int hashval;

	havhash = 1;
	for (cnt = 0; cnt < sizeof xhash; cnt++)
		xhash[cnt] = 0;
	if (v == 0)
		return 0;
	for (pv = v->vec; *pv; pv++, i++) {
		if (pv[0][0] != '/')
			continue;
printf ("dohash: %s\n", *pv);
		if (stat(*pv, &stb) < 0) {
printf ("    ---cannot stat dir %s: %s\n", *pv, strerror(errno));
			continue;
		}
                if (! S_ISDIR(stb.st_mode)) {
printf ("    ---not a directory %s, mode = %#o\n", *pv, stb.st_mode);
			continue;
                }
		dirp = opendir(*pv);
		if (dirp == NULL) {
printf ("    ---cannot open dir %s\n", *pv);
			continue;
                }
		while ((dp = readdir(dirp)) != NULL) {
			if (dp->d_ino == 0)
				continue;
			if (dp->d_name[0] == '.' &&
			    (dp->d_name[1] == '\0' ||
			     dp->d_name[1] == '.' && dp->d_name[2] == '\0'))
				continue;
			hashval = hash(hashname(dp->d_name), i);
			bis(xhash, hashval);
		}
		closedir(dirp);
printf ("    ---done %s\n", *pv);
	}
	return 0;
}

dounhash()
{

	havhash = 0;
}

#ifdef VFORK
hashstat()
{

	if (hits+misses)
		printf("%d hits, %d misses, %d%%\n",
			hits, misses,
			(int)(100L * hits / (hits + misses)));
}
#endif

/*
 * Hash a command name.
 */
hashname(cp)
	register char *cp;
{
	register long h = 0;

	while (*cp)
		h = hash(h, *cp++);
	return ((int) h);
}
