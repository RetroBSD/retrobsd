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
#include <sys/param.h>
#include <machine/limits.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <ctype.h>
#include <paths.h>
#include <stdlib.h>
#include <unistd.h>
#include "chpass.h"

/* ARGSUSED */
int
p_login(p, pw, ep)
	char *p;
	struct passwd *pw;
	struct entry *ep;
{
	if (!*p) {
		(void)fprintf(stderr, "chpass: empty login field.\n");
		return(1);
	}
	if (*p == '-') {
		(void)fprintf(stderr,
		    "chpass: login names may not begin with a hyphen.\n");
		return(1);
	}
	if (!(pw->pw_name = strdup(p))) {
		(void)fprintf(stderr, "chpass: can't save entry.\n");
		return(1);
	}
	if (index(p, '.'))
		(void)fprintf(stderr,
		    "chpass: \'.\' is dangerous in a login name.\n");
	for (; *p; ++p)
		if (isupper(*p)) {
			(void)fprintf(stderr,
			    "chpass: upper-case letters are dangerous in a login name.\n");
			break;
		}
	return(0);
}

/* ARGSUSED */
int
p_passwd(p, pw, ep)
	char *p;
	struct passwd *pw;
	struct entry *ep;
{
	if (!*p)
		pw->pw_passwd = "";	/* "NOLOGIN"; */
	else if (!(pw->pw_passwd = strdup(p))) {
		(void)fprintf(stderr, "chpass: can't save password entry.\n");
		return(1);
	}

	return(0);
}

/* ARGSUSED */
int
p_uid(p, pw, ep)
	register char *p;
	struct passwd *pw;
	struct entry *ep;
{
	int id;

	if (!*p) {
		(void)fprintf(stderr, "chpass: empty uid field.\n");
		return(1);
	}
	if (!isdigit(*p)) {
		(void)fprintf(stderr, "chpass: illegal uid.\n");
		return(1);
	}
	id = atoi(p);
	if ((u_int)id > USHRT_MAX) {
		(void)fprintf(stderr, "chpass: %d > max uid value (%d).\n",
		    id, USHRT_MAX);
		return(1);
	}
	pw->pw_uid = id;
	return(0);
}

/* ARGSUSED */
int
p_gid(p, pw, ep)
	register char *p;
	struct passwd *pw;
	struct entry *ep;
{
	struct group *gr;
	int id;

	if (!*p) {
		(void)fprintf(stderr, "chpass: empty gid field.\n");
		return(1);
	}
	if (!isdigit(*p)) {
		if (!(gr = getgrnam(p))) {
			(void)fprintf(stderr,
			    "chpass: unknown group %s.\n", p);
			return(1);
		}
		pw->pw_gid = gr->gr_gid;
		return(0);
	}
	id = atoi(p);
	if ((u_int)id > USHRT_MAX) {
		(void)fprintf(stderr, "chpass: %d > max gid value (%d).\n",
		    id, USHRT_MAX);
		return(1);
	}
	pw->pw_gid = id;
	return(0);
}

/* ARGSUSED */
int
p_gecos(p, pw, ep)
	char *p;
	struct passwd *pw;
	struct entry *ep;
{
	if (!*p)
		ep->save = "";
	else if (!(ep->save = strdup(p))) {
		(void)fprintf(stderr, "chpass: can't save entry.\n");
		return(1);
	}
	return(0);
}

/* ARGSUSED */
int
p_hdir(p, pw, ep)
	char *p;
	struct passwd *pw;
	struct entry *ep;
{
	if (!*p) {
		(void)fprintf(stderr, "chpass: empty home directory field.\n");
		return(1);
	}
	if (!(pw->pw_dir = strdup(p))) {
		(void)fprintf(stderr, "chpass: can't save entry.\n");
		return(1);
	}
	return(0);
}

/* ARGSUSED */
int
p_shell(p, pw, ep)
	register char *p;
	struct passwd *pw;
	struct entry *ep;
{
	register char *sh, *t;

	if (!*p) {
		pw->pw_shell = _PATH_BSHELL;
		return(0);
	}
	setusershell();
	for (;;) {
		if (!(sh = getusershell())) {
			/* only admin can set "restricted" shells */
			if (!uid)
				break;
			(void)fprintf(stderr,
			    "chpass: %s: non-standard shell.\n", p);
			return(1);
		}
		if (!strcmp(p, sh))
			break;
		/* allow just shell name */
		if ((t = rindex(sh, '/')) && !strcmp(p, t)) {
			p = t;
			break;
		}
	}
	if (!(pw->pw_shell = strdup(p))) {
		(void)fprintf(stderr, "chpass: can't save entry.\n");
		return(1);
	}
	return(0);
}
