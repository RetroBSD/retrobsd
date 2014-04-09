/*
 *	Program Name:   symcompact.c
 *	Date: December 3, 1994
 *	Author: S.M. Schultz
 *
 *	-----------------   Modification History   ---------------
 *      Version Date            Reason For Modification
 *      1.0     03Dec94         1. Initial release into the public domain.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/dir.h>

static	int	oct, status, fflag, rflag;
static	unsigned set, clear;
static	struct	stat st;

/* from ../ls/ */
extern unsigned string_to_flags (char **stringp, unsigned *setp, unsigned *clrp);

void
die(char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);
	exit(1);
}

int
warning(msg)
	char *msg;
{
	if (! fflag)
		fprintf(stderr, "chflags: %s: %s\n", msg, strerror(errno));
	return ! fflag;
}

int
newflags(flags)
	int flags;
{
	if (oct)
		flags = set;
	else {
		flags |= set;
		flags &= clear;
	}
	return flags;
}

int
recurse(dir, savedir)
	char	*dir;
	int	savedir;
{
	register DIR *dirp;
	register struct direct *dp;
	int ecode;

	if (chdir(dir) < 0) {
		warning(dir);
		return(1);
	}
	if ((dirp = opendir(".")) == NULL) {
		warning(dir);
		return(1);
	}
	dp = readdir(dirp);
	dp = readdir(dirp); /* read "." and ".." */
	ecode = 0;
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
		if (lstat(dp->d_name, &st) < 0) {
			ecode = warning(dp->d_name);
			if (ecode)
				break;
			continue;
		}
		if ((st.st_mode & S_IFMT) == S_IFDIR) {
			ecode = recurse(dp->d_name, dirfd(dirp));
			if (ecode)
				break;
			continue;
		}
		if ((st.st_mode & S_IFMT) == S_IFLNK)
			continue;
		if (chflags(dp->d_name, newflags(st.st_flags)) < 0 &&
		    (ecode = warning(dp->d_name)))
			break;
	}
        /*
         * Lastly change the flags on the directory we are in before
         * returning to the previous level.
         */
	if (fstat(dirfd(dirp), &st) < 0)
		die("can't fstat .");
	if (fchflags(dirfd(dirp), newflags(st.st_flags)) < 0)
		ecode = warning(dir);
	if (savedir >= 0 && fchdir(savedir) < 0)
		die("Can't fchdir() back to starting directory");
	closedir(dirp);
	return(ecode);
}

static void
usage()
{
	fputs("usage: chflags [-Rf] flags file ...\n", stderr);
	exit(1);
}

int
main(argc, argv)
	int	argc;
	char	*argv[];
{
	register char *p;
	char	*flags, *ep;
	int	ch, fcurdir = -1;
	long	tmp;

	while ((ch = getopt(argc, argv, "Rf")) != EOF) {
		switch (ch) {
                case 'R':
                        rflag++;
                        break;
                case 'f':
                        fflag++;
                        break;
                case '?':
                default:
                        usage();
                }
        }
	argv += optind;
	argc -= optind;
	if (argc < 2)
		usage();

	flags = *argv++;
	if (*flags >= '0' && *flags <= '7') {
		tmp = strtol(flags, &ep, 8);
		if (tmp < 0 || tmp >= 64L*1024*1024 || *ep)
			die("invalid flags: %s", flags);
		oct = 1;
		set = tmp;
	} else {
		if (string_to_flags(&flags, &set, &clear))
			die("invalid flag: %s", flags);
		clear = ~clear;
		oct = 0;
	}

	if (rflag) {
		fcurdir = open(".", O_RDONLY);
		if (fcurdir < 0)
			die("Can't open .");
	}

	while ((p = *argv++)) {
		if (lstat(p, &st) < 0) {
			status |= warning(p);
			continue;
		}
		if (rflag && (st.st_mode & S_IFMT) == S_IFDIR) {
			status |= recurse(p, fcurdir);
			continue;
		}
		if ((st.st_mode & S_IFMT) == S_IFLNK && stat(p, &st) < 0) {
			status |= warning(p);
			continue;
		}
		if (chflags(p, newflags(st.st_flags)) < 0) {
			status |= warning(p);
			continue;
		}
	}
	if (fcurdir >= 0)
                close(fcurdir);
	exit(status);
}
