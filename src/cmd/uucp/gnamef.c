#include "uucp.h"

/*LINTLIBRARY*/

/*
 *	get next file name from directory
 *
 *	return codes:
 *		0  -  end of directory read
 *		1  -  returned name
 */
int gnamef(dirp, filename)
register DIR *dirp;
register char *filename;
{
	register struct direct *dentp;

	for (;;) {
		if ((dentp = readdir(dirp)) == NULL) {
			return 0;
		}
		if (dentp->d_ino != 0)
			break;
	}

	/* Truncate filename.  This may become a problem someday. */
	strncpy(filename, dentp->d_name, NAMESIZE-1);
	filename[NAMESIZE-1] = '\0';
	DEBUG(99,"gnamef returns %s\n",filename);
	return 1;
}
