/*
 * Copyright (c) 1988 The Regents of the University of California.
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
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <stdio.h>
#include <pwd.h>
#include <strings.h>
#include <fcntl.h>

static FILE *_pw_fp;
static struct passwd _pw_entry;
static int _pw_stayopen;
static char *_pw_file = _PATH_PASSWD;

#define	MAXLINELENGTH	256
static char line[MAXLINELENGTH];

static int
start_pw()
{
	if (_pw_fp) {
		rewind(_pw_fp);
		return(1);
	}
	_pw_fp = fopen(_pw_file, "r");
	if (_pw_fp)
		return(1);
	return(0);
}

static int
scanpw()
{
	register char *cp;
	char	*bp;
	register int ch;

	for (;;) {
		if (!(fgets(line, sizeof(line), _pw_fp)))
			return(0);
		/* skip lines that are too big */
		cp = strchr(line, '\n');
		if (! cp) {
			while ((ch = fgetc(_pw_fp)) != '\n' && ch != EOF)
				;
			continue;
		}
		*cp = '\0';
		bp = line;
		_pw_entry.pw_name = strsep(&bp, ":");
		_pw_entry.pw_passwd = strsep(&bp, ":");
		cp = strsep(&bp, ":");
		if (! cp)
			continue;
		_pw_entry.pw_uid = atoi(cp);
		cp = strsep(&bp, ":");
		if (! cp)
			continue;
		_pw_entry.pw_gid = atoi(cp);
		_pw_entry.pw_gecos = strsep(&bp, ":");
		_pw_entry.pw_dir = strsep(&bp, ":");
		_pw_entry.pw_shell = strsep(&bp, ":");
		if (!_pw_entry.pw_shell)
			continue;
		return(1);
	}
	/* NOTREACHED */
}

static void
getpw()
{
	static char pwbuf[50];
	off_t lseek();
	long pos;
	int fd, n;
	register char *p;

	if (geteuid())
		return;
	/*
	 * special case; if it's the official password file, look in
	 * the master password file, otherwise, look in the file itself.
	 */
	p = strcmp(_pw_file, _PATH_PASSWD) == 0 ? _PATH_SHADOW : _pw_file;
	if ((fd = open(p, O_RDONLY, 0)) < 0)
		return;
	pos = atol(_pw_entry.pw_passwd);
	if (lseek(fd, pos, L_SET) != pos)
		goto bad;
	if ((n = read(fd, pwbuf, sizeof(pwbuf) - 1)) < 0)
		goto bad;
	pwbuf[n] = '\0';
	for (p = pwbuf; *p; ++p)
		if (*p == ':') {
			*p = '\0';
			_pw_entry.pw_passwd = pwbuf;
			break;
		}
bad:	(void)close(fd);
}

struct passwd *
getpwent()
{
	register int rval;

	if (!_pw_fp && !start_pw())
		return((struct passwd *)NULL);
	rval = scanpw();
	if (! rval)
	        return 0;
	getpw();
	return &_pw_entry;
}

struct passwd *
getpwnam(nam)
	char *nam;
{
	register int rval;

	if (!start_pw())
		return((struct passwd *)NULL);
        for (rval = 0; scanpw();) {
                if (!strcmp(nam, _pw_entry.pw_name)) {
                        rval = 1;
                        break;
                }
        }
	if (!_pw_stayopen)
		endpwent();
	if (! rval)
	        return 0;
	getpw();
	return &_pw_entry;
}

struct passwd *
getpwuid(uid)
	int uid;
{
	register int rval;

	if (!start_pw())
		return((struct passwd *)NULL);
        for (rval = 0; scanpw();) {
                if (_pw_entry.pw_uid == uid) {
                        rval = 1;
                        break;
                }
        }
	if (!_pw_stayopen)
		endpwent();
	if (! rval)
	        return 0;
	getpw();
	return &_pw_entry;
}

int
setpwent()
{
	return(setpassent(0));
}

int
setpassent(stayopen)
	int stayopen;
{
	if (!start_pw())
		return(0);
	_pw_stayopen = stayopen;
	return(1);
}

void
endpwent()
{
	if (_pw_fp) {
		(void)fclose(_pw_fp);
		_pw_fp = 0;
	}
}

void
setpwfile(file)
	char *file;
{
	_pw_file = file;
}
