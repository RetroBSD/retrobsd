/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * initgroups
 */
#include <stdio.h>
#include <sys/param.h>
#include <grp.h>
#include <string.h>

int
initgroups(char *uname, int agroup)
{
	gid_t groups[NGROUPS];
	register struct group *grp;
	register int i, ngroups = 0;

	if (agroup >= 0)
		groups[ngroups++] = agroup;
	setgrent();
	while ((grp = getgrent())) {
		if (grp->gr_gid == agroup)
			continue;
		for (i = 0; grp->gr_mem[i]; i++)
			if (!strcmp(grp->gr_mem[i], uname)) {
				if (ngroups == NGROUPS) {
fprintf(stderr, "initgroups: %s is in too many groups\n", uname);
					goto toomany;
				}
				groups[ngroups++] = grp->gr_gid;
			}
	}
toomany:
	endgrent();
	if (setgroups(ngroups, groups) < 0) {
		perror("setgroups");
		return (-1);
	}
	return (0);
}
