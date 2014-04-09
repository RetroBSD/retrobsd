/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/param.h>
#include <sys/dir.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

/*
 * open a directory.
 */
DIR *
opendir(name)
	const char *name;
{
	register DIR *dirp;
	register int fd;

	if ((fd = open(name, 0)) == -1)
		return NULL;
	if ((dirp = (DIR *)malloc(sizeof(DIR))) == NULL) {
		close (fd);
		return NULL;
	}
	dirp->dd_fd = fd;
	dirp->dd_loc = 0;
	return dirp;
}
