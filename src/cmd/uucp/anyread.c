#include "uucp.h"
#include <sys/stat.h>

/*LINTLIBRARY*/

/*
 *	anyread		check if anybody can read
 *	return SUCCESS ok: FAIL not ok
 */
int anyread(file)
char *file;
{
	struct stat s;

	if (stat(subfile(file), &s) < 0)
		/* for security check a non existant file is readable */
		return SUCCESS;
	if (!(s.st_mode & ANYREAD))
		return FAIL;
	return SUCCESS;
}
