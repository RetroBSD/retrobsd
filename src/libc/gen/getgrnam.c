#include <sys/types.h>
#include <grp.h>
#include <string.h>

struct group *
getgrnam(name)
        register const char *name;
{
	register struct group *p;

	setgrent();
	while ((p = getgrent()) && strcmp(p->gr_name, name));
	endgrent();
	return(p);
}
