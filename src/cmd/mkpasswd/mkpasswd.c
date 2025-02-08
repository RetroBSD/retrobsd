/*
 * Copyright (c) 1980, 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <ndbm.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

static FILE *_pw_fp;
static struct passwd _pw_passwd;
static off_t offset;

#define	MAXLINELENGTH	256
static char line[MAXLINELENGTH];

static void usage(void);
static void rmall(char *fname);

/* from libc/gen/getpwent.c */

static int
scanpw()
{
	register char *cp;
	char	*bp;

	for (;;) {
		offset = ftell(_pw_fp);
		if (!(fgets(line, sizeof(line), _pw_fp)))
			return(0);
		bp = line;
		/* skip lines that are too big */
		if (!(cp = index(line, '\n'))) {
			int ch;

			while ((ch = getc(_pw_fp)) != '\n' && ch != EOF)
				;
			continue;
		}
		*cp = '\0';
		_pw_passwd.pw_name = strsep(&bp, ":");
		_pw_passwd.pw_passwd = strsep(&bp, ":");
		offset += _pw_passwd.pw_passwd - line;
		if (!(cp = strsep(&bp, ":")))
			continue;
		_pw_passwd.pw_uid = atoi(cp);
		if (!(cp = strsep(&bp, ":")))
			continue;
		_pw_passwd.pw_gid = atoi(cp);
		_pw_passwd.pw_gecos = strsep(&bp, ":");
		_pw_passwd.pw_dir = strsep(&bp, ":");
		_pw_passwd.pw_shell = strsep(&bp, ":");
		return(1);
	}
	/* NOTREACHED */
}

/*
 * Mkpasswd does two things -- use the ``arg'' file to create ``arg''.{pag,dir}
 * for ndbm, and, if the -p flag is on, create a password file in the original
 * format.  It doesn't use the getpwent(3) routines because it has to figure
 * out offsets for the encrypted passwords to put in the dbm files.  One other
 * problem is that, since the addition of shadow passwords, getpwent(3) has to
 * use the dbm databases rather than simply scanning the actual file.  This
 * required the addition of a flag field to the dbm database to distinguish
 * between a record keyed by name, and one keyed by uid.
 */
int main(argc, argv)
	int argc;
	char **argv;
{
	extern int errno, optind;
	register char *flag, *p, *t;
	register int makeold;
	FILE *oldfp;
	DBM *dp;
	datum key, content;
	int ch;
	char buf[256], nbuf[50];

	makeold = 0;
	while ((ch = getopt(argc, argv, "pv")) != EOF)
		switch(ch) {
		case 'p':			/* create ``password.orig'' */
			makeold = 1;
			/* FALLTHROUGH */
		case 'v':			/* backward compatible */
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	if (!(_pw_fp = fopen(*argv, "r"))) {
		(void)fprintf(stderr,
		    "mkpasswd: %s: can't open for reading.\n", *argv);
		exit(1);
	}

	rmall(*argv);
	(void)umask(0);

	/* open old password format file, dbm files */
	if (makeold) {
		int oldfd;

		(void)sprintf(buf, "%s.orig", *argv);
		if ((oldfd = open(buf, O_WRONLY|O_CREAT|O_EXCL, 0644)) < 0) {
			(void)fprintf(stderr, "mkpasswd: %s: %s\n", buf,
			    strerror(errno));
			exit(1);
		}
		if (!(oldfp = fdopen(oldfd, "w"))) {
			(void)fprintf(stderr, "mkpasswd: %s: fdopen failed.\n",
			    buf);
			exit(1);
		}
	}
	if (!(dp = dbm_open(*argv, O_WRONLY|O_CREAT|O_EXCL, 0644))) {
		(void)fprintf(stderr, "mkpasswd: %s: %s\n", *argv,
		    strerror(errno));
		exit(1);
	}

	content.dptr = buf;
	while (scanpw()) {
		/* create dbm entry */
		p = buf;
#define	COMPACT(e)	t = e; while ((*p++ = *t++));
		COMPACT(_pw_passwd.pw_name);
		(void)sprintf(nbuf, "%ld", offset);
		COMPACT(nbuf);
		bcopy((char *)&_pw_passwd.pw_uid, p, sizeof(int));
		p += sizeof(int);
		bcopy((char *)&_pw_passwd.pw_gid, p, sizeof(int));
		p += sizeof(int);
		COMPACT(_pw_passwd.pw_gecos);
		COMPACT(_pw_passwd.pw_dir);
		COMPACT(_pw_passwd.pw_shell);
		flag = p;
		*p++ = _PW_KEYBYNAME;
		content.dsize = p - buf;
#ifdef debug
		(void)printf("store %s, uid %d\n", _pw_passwd.pw_name,
		    _pw_passwd.pw_uid);
#endif
		key.dptr = _pw_passwd.pw_name;
		key.dsize = strlen(_pw_passwd.pw_name);
		if (dbm_store(dp, key, content, DBM_INSERT) < 0)
			goto bad;
		key.dptr = (char *)&_pw_passwd.pw_uid;
		key.dsize = sizeof(int);
		*flag = _PW_KEYBYUID;
		if (dbm_store(dp, key, content, DBM_INSERT) < 0)
			goto bad;

		/* create original format password file entry */
		if (!makeold)
			continue;
		fprintf(oldfp, "%s:%d:%d:%d:%s:%s:%s\n", _pw_passwd.pw_name,
                    offset, _pw_passwd.pw_uid, _pw_passwd.pw_gid,
                    _pw_passwd.pw_gecos, _pw_passwd.pw_dir,
                    _pw_passwd.pw_shell);
	}
	dbm_close(dp);
	exit(0);

bad:	(void)fprintf(stderr, "mkpasswd: dbm_store failed.\n");
	rmall(*argv);
	exit(1);
}

void rmall(fname)
	char *fname;
{
	register char *p;
	char buf[MAXPATHLEN];

	for (p = strcpy(buf, fname); *p; ++p);
	bcopy(".pag", p, 5);
	(void)unlink(buf);
	bcopy(".dir", p, 5);
	(void)unlink(buf);
	bcopy(".orig", p, 6);
	(void)unlink(buf);
}

void usage()
{
	(void)fprintf(stderr, "usage: mkpasswd [-p] passwd_file\n");
	exit(1);
}
