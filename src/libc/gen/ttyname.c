/*
 * ttyname(f): return "/dev/ttyXX" which the the name of the
 * tty belonging to file f.
 *  NULL if it is not a tty
 */
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

static	char	dev[]	= "/dev/";

char *
ttyname(f)
{
	struct stat fsb;
	struct stat tsb;
	register struct direct *db;
	register DIR *df;
	static char rbuf[32];

	if (isatty(f)==0)
		return 0;
	if (fstat(f, &fsb) < 0)
		return 0;
	if ((fsb.st_mode&S_IFMT) != S_IFCHR)
		return 0;
        df = opendir(dev);
	if (! df)
		return 0;
	while ((db = readdir(df))) {
		if (db->d_ino != fsb.st_ino)
			continue;
		strcpy(rbuf, dev);
		strcat(rbuf, db->d_name);
		if (stat(rbuf, &tsb) < 0)
			continue;
		if (tsb.st_dev == fsb.st_dev && tsb.st_ino == fsb.st_ino) {
			closedir(df);
			return(rbuf);
		}
	}
	closedir(df);
	return 0;
}
