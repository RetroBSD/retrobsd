#include "uucp.h"
#include <pwd.h>

/*LINTLIBRARY*/

/*
 *	get passwd file info for uid
 *
 *	return codes:  SUCCESS  |  FAIL
 *
 */
int guinfo(uid, name, path)
int uid;
register char *path, *name;
{
	register struct passwd *pwd;
	char *l;

	if ((l = getlogin()) == NULL) {
		l = getenv("USER");
	}
	if (l != NULL) {
		pwd = getpwnam(l);
		if (pwd != NULL && pwd->pw_uid == uid)
			goto setup;
	}
	if ((pwd = getpwuid(uid)) == NULL) {
		/* can not find uid in passwd file */
		*name = '\0';
		*path = '\0';
		return FAIL;
	}

    setup:
	strcpy(path, pwd->pw_dir);
	strcpy(name, pwd->pw_name);
	return SUCCESS;
}

/*
 *	get passwd file info for name
 *
 *	return codes:  SUCCESS  |  FAIL
 */
int gninfo(name, uid, path)
char *path, *name;
int *uid;
{
	register struct passwd *pwd;

	if ((pwd = getpwnam(name)) == NULL) {
		/* can not find name in passwd file */
		*path = '\0';
		return FAIL;
	}

	strcpy(path, pwd->pw_dir);
	*uid = pwd->pw_uid;
	return SUCCESS;
}
