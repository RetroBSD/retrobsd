/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 */
#include "mac.h"
#include "defs.h"

#define DOT     '.'
#define NULL    0
#define SLASH   '/'
#define MAXPWD  256

extern char     longpwd[];

static char cwdname[MAXPWD];
static int didpwd = FALSE;

/*
 * This routine will remove repeated slashes from string.
 */
static
rmslash(string)
	char *string;
{
	register char *pstring;

	pstring = string;
	while(*pstring)
	{
		if(*pstring==SLASH && *(pstring+1)==SLASH)
		{
			/* Remove repeated SLASH's */

			movstr(pstring+1, pstring);
			continue;
		}
		pstring++;
	}

	--pstring;
	if(pstring>string && *pstring==SLASH)
	{
		/* Remove trailing / */

		*pstring = NULL;
	}
	return;
}

cwd(dir)
	register char *dir;
{
	register char *pcwd;
	register char *pdir;

	/* First remove extra /'s */

	rmslash(dir);

	/* Now remove any .'s */

	pdir = dir;
	while(*pdir)                    /* remove /./ by itself */
	{
		if((*pdir==DOT) && (*(pdir+1)==SLASH))
		{
			movstr(pdir+2, pdir);
			continue;
		}
		pdir++;
		while ((*pdir) && (*pdir != SLASH))
			pdir++;
		if (*pdir)
			pdir++;
	}
	if(*(--pdir)==DOT && pdir>dir && *(--pdir)==SLASH)
		*pdir = NULL;


	/* Remove extra /'s */

	rmslash(dir);

	/* Now that the dir is canonicalized, process it */

	if(*dir==DOT && *(dir+1)==NULL)
	{
		return;
	}

	if(*dir==SLASH)
	{
		/* Absolute path */

		pcwd = cwdname;
		didpwd = TRUE;
	}
	else
	{
		/* Relative path */

		if (didpwd == FALSE)
			return;

		pcwd = cwdname + length(cwdname) - 1;
		if(pcwd != cwdname+1)
		{
			*pcwd++ = SLASH;
		}
	}
	while(*dir)
	{
		if(*dir==DOT &&
		   *(dir+1)==DOT &&
		   (*(dir+2)==SLASH || *(dir+2)==NULL))
		{
			/* Parent directory, so backup one */

			if( pcwd > cwdname+2 )
				--pcwd;
			while(*(--pcwd) != SLASH)
				;
			pcwd++;
			dir += 2;
			if(*dir==SLASH)
			{
				dir++;
			}
			continue;
		}
		*pcwd++ = *dir++;
		while((*dir) && (*dir != SLASH))
			*pcwd++ = *dir++;
		if (*dir)
			*pcwd++ = *dir++;
	}
	*pcwd = NULL;

	--pcwd;
	if(pcwd>cwdname && *pcwd==SLASH)
	{
		/* Remove trailing / */

		*pcwd = NULL;
	}
	return;
}

/*
 * Find the current directory the hard way.
 */
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>

static char dotdots[] =
"../../../../../../../../../../../../../../../../../../../../../../../..";

extern struct direct	*getdir();
extern char		*movstrn();

static
pwd()
{
	struct stat		cdir;	/* current directory status */
	struct stat		tdir;
	struct stat		pdir;	/* parent directory status */
	int				pdfd;	/* parent directory file descriptor */

	struct direct	*dir;
	char 			*dot = dotdots + sizeof(dotdots) - 3;
	int				index = sizeof(dotdots) - 2;
	int				cwdindex = MAXPWD - 1;
	int 			i;

	cwdname[cwdindex] = 0;
	dotdots[index] = 0;

	if(stat(dot, &pdir) < 0)
	{
		error("pwd: cannot stat .");
	}

	dotdots[index] = '.';

	for(;;)
	{
		cdir = pdir;

		if ((pdfd = open(dot, 0)) < 0)
		{
			error("pwd: cannot open ..");
		}

		if(fstat(pdfd, &pdir) < 0)
		{
			close(pdfd);
			error("pwd: cannot stat ..");
		}

		if(cdir.st_dev == pdir.st_dev)
		{
			if(cdir.st_ino == pdir.st_ino)
			{
				didpwd = TRUE;
				close(pdfd);
				if (cwdindex == (MAXPWD - 1))
					cwdname[--cwdindex] = SLASH;

				movstr(&cwdname[cwdindex], cwdname);
				return;
			}

			do
			{
				if ((dir = getdir(pdfd)) == NIL)
				{
					close(pdfd);
					reset_dir();
					error("pwd: read error in ..");
				}
			}
			while (dir->d_ino != cdir.st_ino);
		}
		else
		{
			char name[512];

			movstr(dot, name);
			i = length(name) - 1;

			name[i++] = '/';

			do
			{
				if ((dir = getdir(pdfd)) == NIL)
				{
					close(pdfd);
					reset_dir();
					error("pwd: read error in ..");
				}
				*(movstrn(dir->d_name, &name[i], MAXNAMLEN)) = 0;
				stat(name, &tdir);
			}
			while(tdir.st_ino != cdir.st_ino || tdir.st_dev != cdir.st_dev);
		}
		close(pdfd);
		reset_dir();

		for (i = 0; i < MAXNAMLEN; i++)
			if (dir->d_name[i] == 0)
				break;

		if (i > cwdindex - 1)
				error(longpwd);
		else
		{
			cwdindex -= i;
			movstrn(dir->d_name, &cwdname[cwdindex], i);
			cwdname[--cwdindex] = SLASH;
		}

		dot -= 3;
		if (dot<dotdots)
			error(longpwd);
	}
}

/*
 * Print the current working directory.
 */
prcwd()
{
	if (didpwd == FALSE)
		pwd();
	prs_buff(cwdname);
}

cwdprint()
{
	prcwd();
	prc_buff(NL);
}
