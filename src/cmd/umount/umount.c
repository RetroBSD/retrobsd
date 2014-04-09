/*-
 * Copyright (c) 1980, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <fstab.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum { MNTON, MNTFROM } mntwhat;

int	fake, fflag, vflag, allflag, *typelist;
char	*nfshost;

int	 fsnametotype();
char	*getmntname();
void	 maketypelist();
int	 selected();
int	 namematch();
int	 umountall();
int	 umountfs();
void	 usage();

int
main(argc, argv)
	int argc;
	register char *argv[];
{
	int ch, errs;

	/* Start disks transferring immediately. */
	sync();

	while ((ch = getopt(argc, argv, "aFft:v")) != EOF)
		switch (ch) {
		case 'a':
			allflag = 1;
			break;
		case 'F':
			fake = 1;
			break;
		case 'f':
#ifdef	notnow
			fflag = MNT_FORCE;
#endif
			break;
		case 't':
			maketypelist(optarg);
			break;
		case 'v':
			vflag = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if (argc == 0 && !allflag || argc != 0 && allflag)
		usage();

	if (allflag) {
		if (setfsent() == 0)
			err(1, "%s", _PATH_FSTAB);
		errs = umountall();
	} else
		for (errs = 0; *argv != NULL; ++argv)
			if (umountfs(*argv) == 0)
				errs = 1;
	exit(errs);
}

int
umountall()
{
	register struct fstab *fs;
	int rval, type;
	register char *cp;

	while ((fs = getfsent()) != NULL) {
		/* Ignore the root. */
		if (strcmp(fs->fs_file, "/") == 0)
			continue;
		/*
		 * !!!
		 * Historic practice: ignore unknown FSTAB_* fields.
		 */
		if (strcmp(fs->fs_type, FSTAB_RW) &&
		    strcmp(fs->fs_type, FSTAB_RO) &&
		    strcmp(fs->fs_type, FSTAB_RQ))
			continue;
		/* If an unknown file system type, complain. */
		if ((type = fsnametotype(fs->fs_vfstype)) == MOUNT_NONE) {
			warnx("%s: unknown mount type", fs->fs_vfstype);
			continue;
		}
		if (!selected(type))
			continue;

		/*
		 * We want to unmount the file systems in the reverse order
		 * that they were mounted.  So, we save off the file name
		 * in some allocated memory, and then call recursively.
		 */
		cp = (char *)malloc((size_t)strlen(fs->fs_file) + 1);
		if (cp == NULL)
			err(1, NULL);
		(void)strcpy(cp, fs->fs_file);
		rval = umountall();
		return (umountfs(cp) || rval);
	}
	return (0);
}

int
umountfs(name)
	char *name;
{
	struct stat sb;
	int type;
	register char *mntpt;

	if (stat(name, &sb) < 0) {
		if (((mntpt = getmntname(name, MNTFROM, &type)) == NULL) &&
		    ((mntpt = getmntname(name, MNTON, &type)) == NULL)) {
			warnx("%s: not currently mounted", name);
			return (1);
		}
	} else if ((sb.st_mode & S_IFMT) == S_IFBLK) {
		if ((mntpt = getmntname(name, MNTON, &type)) == NULL) {
			warnx("%s: not currently mounted", name);
			return (1);
		}
	} else if ((sb.st_mode & S_IFMT) == S_IFDIR) {
		mntpt = name;
		if ((name = getmntname(mntpt, MNTFROM, &type)) == NULL) {
		        if (! allflag || vflag)
                                warnx("%s: not currently mounted", mntpt);
			return (1);
		}
	} else {
		warnx("%s: not a directory or special device", name);
		return (1);
	}

	if (!selected(type))
		return (0);

	if (vflag)
		(void)printf("%s: unmount from %s\n", name, mntpt);
	if (fake)
		return (0);

	if (umount(name) < 0) {
		warn("%s on %s", name, mntpt);
		return (1);
	}

	return (0);
}

char *
getmntname(name, what, type)
	char *name;
	mntwhat what;
	int *type;
{
	struct statfs *mntbuf;
	register int i, mntsize;

	if ((mntsize = getmntinfo(&mntbuf, MNT_NOWAIT)) == 0) {
		warn("getmntinfo");
		return (NULL);
	}
	for (i = 0; i < mntsize; i++) {
		if ((what == MNTON) && !strcmp(mntbuf[i].f_mntfromname, name)) {
			if (type)
				*type = mntbuf[i].f_type;
			return (mntbuf[i].f_mntonname);
		}
		if ((what == MNTFROM) && !strcmp(mntbuf[i].f_mntonname, name)) {
			if (type)
				*type = mntbuf[i].f_type;
			return (mntbuf[i].f_mntfromname);
		}
	}
	return (NULL);
}

static enum { IN_LIST, NOT_IN_LIST } which;

int
selected(type)
	int type;
{
	/* If no type specified, it's always selected. */
	if (typelist == NULL)
		return (1);
	for (; *typelist != MOUNT_NONE; ++typelist)
		if (type == *typelist)
			return (which == IN_LIST ? 1 : 0);
	return (which == IN_LIST ? 0 : 1);
}

void
maketypelist(fslist)
	register char *fslist;
{
	register int *av, i;
	char *nextcp;

	if ((fslist == NULL) || (fslist[0] == '\0'))
		errx(1, "empty type list");

	/*
	 * XXX
	 * Note: the syntax is "noxxx,yyy" for no xxx's and
	 * no yyy's, not the more intuitive "noyyy,noyyy".
	 */
	if (fslist[0] == 'n' && fslist[1] == 'o') {
		fslist += 2;
		which = NOT_IN_LIST;
	} else
		which = IN_LIST;

	/* Count the number of types. */
	for (i = 0, nextcp = fslist; *nextcp != NULL; ++nextcp)
		if (*nextcp == ',')
			i++;

	/* Build an array of that many types. */
	if ((av = typelist = (int *)malloc((i + 2) * sizeof(int))) == NULL)
		err(1, NULL);
	for (i = 0; fslist != NULL; fslist = nextcp, ++i) {
		if ((nextcp = strchr(fslist, ',')) != NULL)
			*nextcp++ = '\0';
		av[i] = fsnametotype(fslist);
		if (av[i] == MOUNT_NONE)
			errx(1, "%s: unknown mount type", fslist);
	}
	/* Terminate the array. */
	av[i++] = MOUNT_NONE;
}

int
fsnametotype(name)
	char *name;
{
	static char *namelist[] = INITMOUNTNAMES;
	register char **cp;

	for (cp = namelist; *cp; ++cp)
		if (strcmp(name, *cp) == 0)
			return (cp - namelist);
	return (MOUNT_NONE);
}

void
usage()
{
	(void)fprintf(stderr,
	    "usage: %s\n       %s\n",
	    "umount [-fv] [-t fstypelist] special | node",
	    "umount -a[fv] [-t fstypelist]");
	exit(1);
}
