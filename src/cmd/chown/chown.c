/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * chown [-fR] uid[.gid] file ...
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <pwd.h>
#include <sys/dir.h>
#include <grp.h>
#include <strings.h>

static	char	*fchdirmsg = "Can't fchdir() back to starting directory";
struct	passwd *pwd;
struct	passwd *getpwnam();
struct	stat stbuf;
uid_t	uid;
int	status;
int	fflag;
int	rflag;

main(argc, argv)
	char *argv[];
{
	register int c;
	register gid_t gid;
	register char *cp, *group;
	struct group *grp;
	int	fcurdir;

	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		for (cp = &argv[0][1]; *cp; cp++) switch (*cp) {

		case 'f':
			fflag++;
			break;

		case 'R':
			rflag++;
			break;

		default:
			fatal(255, "unknown option: %c", *cp);
		}
		argv++, argc--;
	}
	if (argc < 2) {
		fprintf(stderr, "usage: chown [-fR] owner[.group] file ...\n");
		exit(-1);
	}
	gid = -1;
	group = index(argv[0], '.');
	if (group != NULL) {
		*group++ = '\0';
		if (!isnumber(group)) {
			if ((grp = getgrnam(group)) == NULL)
				fatal(255, "unknown group: %s",group);
			gid = grp -> gr_gid;
			(void) endgrent();
		} else if (*group != '\0')
			gid = atoi(group);
	}
	if (!isnumber(argv[0])) {
		if ((pwd = getpwnam(argv[0])) == NULL)
			fatal(255, "unknown user id: %s",argv[0]);
		uid = pwd->pw_uid;
	} else
		uid = atoi(argv[0]);

	fcurdir = open(".", O_RDONLY);
	if	(fcurdir < 0)
		fatal(255, "Can't open .");

	for (c = 1; c < argc; c++) {
		/* do stat for directory arguments */
		if (lstat(argv[c], &stbuf) < 0) {
			status += Perror(argv[c]);
			continue;
		}
		if (rflag && ((stbuf.st_mode&S_IFMT) == S_IFDIR)) {
			status += chownr(argv[c], uid, gid, fcurdir);
			continue;
		}
		if (chown(argv[c], uid, gid)) {
			status += Perror(argv[c]);
			continue;
		}
	}
	exit(status);
}

isnumber(s)
	char *s;
{
	register c;

	while(c = *s++)
		if (!isdigit(c))
			return (0);
	return (1);
}

chownr(dir, uid, gid, savedir)
	char *dir;
{
	register DIR *dirp;
	register struct direct *dp;
	struct stat st;
	int ecode;

	/*
	 * Change what we are given before doing it's contents.
	 */
	if (chown(dir, uid, gid) < 0 && Perror(dir))
		return (1);
	if (chdir(dir) < 0) {
		Perror(dir);
		return (1);
	}
	if ((dirp = opendir(".")) == NULL) {
		Perror(dir);
		return (1);
	}
	dp = readdir(dirp);
	dp = readdir(dirp); /* read "." and ".." */
	ecode = 0;
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
		if (lstat(dp->d_name, &st) < 0) {
			ecode = Perror(dp->d_name);
			if (ecode)
				break;
			continue;
		}
		if ((st.st_mode&S_IFMT) == S_IFDIR) {
			ecode = chownr(dp->d_name, uid, gid, dirfd(dirp));
			if (ecode)
				break;
			continue;
		}
		if (chown(dp->d_name, uid, gid) < 0 &&
		    (ecode = Perror(dp->d_name)))
			break;
	}
	if	(fchdir(savedir) < 0)
		fatal(255, fchdirmsg);
	closedir(dirp);
	return (ecode);
}

error(fmt, a)
	char *fmt, *a;
{

	if (!fflag) {
		fprintf(stderr, "chown: ");
		fprintf(stderr, fmt, a);
		putc('\n', stderr);
	}
	return (!fflag);
}

fatal(status, fmt, a)
	int status;
	char *fmt, *a;
{

	fflag = 0;
	(void) error(fmt, a);
	exit(status);
}

Perror(s)
	char *s;
{

	if (!fflag) {
		fprintf(stderr, "chown: ");
		perror(s);
	}
	return (!fflag);
}
