/*
 * Copyright (c) 1987 Regents of the University of California.
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
#include <sys/dir.h>
#include <sys/utsname.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <paths.h>

#define	NEW_PATH	"/new/man"

#define	NO	0
#define	YES	1

static char	*command,		/* command buffer */
		*defpath,		/* default search path */
		*locpath,		/* local search path */
		*machine,		/* machine type */
		*manpath,		/* current search path */
		*newpath,		/* new search path */
		*pager,			/* requested pager */
		how;			/* how to display */

#define	ALL	0x1			/* show all man pages */
#define	CAT	0x2			/* copy file to stdout */
#define	WHERE	0x4			/* just tell me where */

typedef struct {
	char	*name, *msg;
} MANDIR;

static MANDIR	list1[] = {		/* section one list */
	"cat1", "1st",		"cat8", "8th",		"cat6", "6th",
	"cat.old", "old",	NULL, NULL,
};

static MANDIR	list2[] = {		/* rest of the list */
	"cat2", "2nd",		"cat3", "3rd",		"cat4", "4th",
	"cat5", "5th", 		"cat7", "7th",		"cat3f", "3rd (F)",
	NULL, NULL,
};

static MANDIR	list3[2];		/* single section */

/*
 * cat --
 *	cat out the file
 */
static
cat(fname)
	char *fname;
{
	register int fd, n;
	char buf[BUFSIZ];

	if (!(fd = open(fname, O_RDONLY, 0))) {
		perror("man: open");
		exit(1);
	}
	while ((n = read(fd, buf, sizeof(buf))) > 0)
		if (write(1, buf, n) != n) {
			perror("man: write");
			exit(1);
		}
	if (n == -1) {
		perror("man: read");
		exit(1);
	}
	(void)close(fd);
}

/*
 * add --
 *	add a file name to the list for future paging
 */
static
add(fname)
	char *fname;
{
	static u_int buflen;
	static int len;
	static char *cp;
	int flen;

	if (!command) {
		if (!(command = malloc(buflen = 1024))) {
			fputs("man: out of space.\n", stderr);
			exit(1);
		}
		len = strlen(strcpy(command, pager));
		cp = command + len;
	}
	flen = strlen(fname);
	if (len + flen + 2 > buflen) {		/* +2 == space, EOS */
		if (!(command = realloc(command, buflen += 1024))) {
			fputs("man: out of space.\n", stderr);
			exit(1);
		}
		cp = command + len;
	}
	*cp++ = ' ';
	len += flen + 1;			/* +1 = space */
	(void)strcpy(cp, fname);
	cp += flen;
}

/*
 * manual --
 *	given a directory list and a file name find a file that
 *	matches; check ${directory}/${dir}/{file name} and
 *	${directory}/${dir}/${machine}/${file name}.
 */
static
manual(section, name)
	MANDIR *section;
	char *name;
{
	register char *beg, *end;
	register MANDIR *dp;
	register int res;
	char fname[MAXPATHLEN + 1], *index();

	if (strlen(name) > MAXNAMLEN-2)	/* leave room for the ".0" */
		name[MAXNAMLEN-2] = '\0';
	for (beg = manpath, res = 0;; beg = end + 1) {
		if (end = index(beg, ':'))
			*end = '\0';
		for (dp = section; dp->name; ++dp) {
			(void)sprintf(fname, "%s/%s/%s.0", beg, dp->name, name);
			if (access(fname, R_OK)) {
				(void)sprintf(fname, "%s/%s/%s/%s.0", beg,
				    dp->name, machine, name);
				if (access(fname, R_OK))
					continue;
			}
			if (how & WHERE)
				printf("man: found in %s.\n", fname);
			else if (how & CAT)
				cat(fname);
			else
				add(fname);
			if (!(how & ALL))
				return(1);
			res = 1;
		}
		if (!end)
			return(res);
		*end = ':';
	}
	/*NOTREACHED*/
}

/*
 * getsect --
 *	return a point to the section structure for a particular suffix
 */
static MANDIR *
getsect(s)
	char *s;
{
	switch(*s++) {
	case '1':
		if (!*s)
			return(list1);
		break;
	case '2':
		if (!*s) {
			list3[0] = list2[0];
			return(list3);
		}
		break;
	/* sect. 3 requests are for either section 3, or section 3[fF]. */
	case '3':
		if (!*s) {
			list3[0] = list2[1];
			return(list3);
		}
		else if ((*s == 'f'  || *s == 'F') && !*++s) {
			list3[0] = list2[5];
			return(list3);
		}
		break;
	case '4':
		if (!*s) {
			list3[0] = list2[2];
			return(list3);
		}
		break;
	case '5':
		if (!*s) {
			list3[0] = list2[3];
			return(list3);
		}
		break;
	case '6':
		if (!*s) {
			list3[0] = list1[2];
			return(list3);
		}
		break;
	case '7':
		if (!*s) {
			list3[0] = list2[4];
			return(list3);
		}
		break;
	case '8':
		if (!*s) {
			list3[0] = list1[1];
			return(list3);
		}
	}
	return((MANDIR *)NULL);
}

static
man(argv)
	char **argv;
{
	register char *p;
	MANDIR *section;
	int res;

	for (; *argv; ++argv) {
		manpath = defpath;
		section = NULL;
		switch(**argv) {
		case 'l':				/* local */
			/* support the "{l,local,n,new}###"  syntax */
			for (p = *argv; isalpha(*p); ++p);
			if (!strncmp(*argv, "l", p - *argv) ||
			    !strncmp(*argv, "local", p - *argv)) {
				++argv;
				manpath = locpath;
				section = getsect(p);
			}
			break;
		case 'n':				/* new */
			for (p = *argv; isalpha(*p); ++p);
			if (!strncmp(*argv, "n", p - *argv) ||
			    !strncmp(*argv, "new", p - *argv)) {
				++argv;
				manpath = newpath;
				section = getsect(p);
			}
			break;
		/*
		 * old isn't really a separate section of the manual,
		 * and its entries are all in a single directory.
		 */
		case 'o':				/* old */
			for (p = *argv; isalpha(*p); ++p);
			if (!strncmp(*argv, "o", p - *argv) ||
			    !strncmp(*argv, "old", p - *argv)) {
				++argv;
				list3[0] = list1[3];
				section = list3;
			}
			break;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8':
			if (section = getsect(*argv))
				++argv;
		}

		if (*argv) {
			if (section)
				res = manual(section, *argv);
			else {
				res = manual(list1, *argv);
				if (!res || (how & ALL))
					res += manual(list2, *argv);
			}
			if (res || how&WHERE)
				continue;
		}

		fputs("man: ", stderr);
		if (*argv)
			fprintf(stderr, "no entry for %s in the ", *argv);
		else
			fputs("what do you want from the ", stderr);
		if (section)
			fprintf(stderr, "%s section of the ", section->msg);
		if (manpath == locpath)
			fputs("local ", stderr);
		else if (manpath == newpath)
			fputs("new ", stderr);
		if (*argv)
			fputs("manual.\n", stderr);
		else
			fputs("manual?\n", stderr);
		exit(1);
	}
}

/*
 * jump --
 *	strip out flag argument and jump
 */
static
jump(argv, flag, name)
	char **argv, *name;
	register char *flag;
{
	register char **arg;

	argv[0] = name;
	for (arg = argv + 1; *arg; ++arg)
		if (!strcmp(*arg, flag))
			break;
	for (; *arg; ++arg)
		arg[0] = arg[1];
	execvp(name, argv);
	fprintf(stderr, "%s: Command not found.\n", name);
	exit(1);
}

/*
 * This is done in a function by itself because 'uname()' uses a 640
 * structure which we do not want permanently allocated on main()'s stack.
*/
setmachine()
        {
        struct  utsname foo;

        if      (uname(&foo) < 0)
                strcpy(foo.machine, "?");
        machine = strdup(foo.machine);
        }

/*
 * usage --
 *	print usage and die
 */
static
usage()
{
	fputs("usage: man [-] [-a] [-M path] [section] title ...\n", stderr);
	exit(1);
}

main(argc, argv)
	int argc;
	register char **argv;
{
	extern char *optarg;
	extern int optind;
	int ch;

	while ((ch = getopt(argc, argv, "-M:P:afkw")) != EOF)
		switch((char)ch) {
		case '-':
			how |= CAT;
			break;
		case 'M':
		case 'P':		/* backward compatibility */
			defpath = optarg;
			break;
		case 'a':
			how |= ALL;
			break;
		/*
		 * "man -f" and "man -k" are backward contemptible,
		 * undocumented ways of calling whatis(1) and apropos(1).
		 */
		case 'f':
			jump(argv, "-f", "whatis");
			/*NOTREACHED*/
		case 'k':
			jump(argv, "-k", "apropos");
			/*NOTREACHED*/
		/*
		 * Deliberately undocumented; really only useful when
		 * you're moving man pages around.  Not worth adding.
		 */
		case 'w':
			how |= WHERE | ALL;
			break;
		case '?':
		default:
			usage();
		}
	argv += optind;

	if (!*argv)
		usage();

	if (!(how & CAT))
		if (!isatty(1))
			how |= CAT;
		else if (pager = getenv("PAGER")) {
			register char *p;

			/*
			 * if the user uses "more", we make it "more -s"
			 * watch out for PAGER = "mypager /bin/more"
			 */
			for (p = pager; *p && !isspace(*p); ++p);
			for (; p > pager && *p != '/'; --p);
			if (p != pager)
				++p;
			/* make sure it's "more", not "morex" */
			if (!strncmp(p, "more", 4) && (!p[4] || isspace(p[4]))){
				char *opager = pager;
				/*
				 * allocate space to add the "-s"
				 */
				if (!(pager = malloc((u_int)(strlen(opager)
				    + sizeof("-s") + 1)))) {
					fputs("man: out of space.\n", stderr);
					exit(1);
				}
				(void)sprintf(pager, "%s %s", opager, "-s");
			}
		}
		else
			pager = _PATH_MORE " -s";
	if (!(machine = getenv("MACHINE")))
		setmachine();
	if (!defpath && !(defpath = getenv("MANPATH")))
		defpath = _PATH_MAN;
	locpath = _PATH_LOCALMAN;
	newpath = NEW_PATH;
	man(argv);
	/* use system(3) in case someone's pager is "pager arg1 arg2" */
	if (command)
		(void)system(command);
	exit(0);
}
