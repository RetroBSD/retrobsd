/* $Header: util.c,v 7.0.1.2 86/10/20 12:07:46 lwall Exp $ */

/* $Log:	util.c,v $
 * Revision 7.0.1.2  86/10/20  12:07:46  lwall
 * Made all exits reset tty.
 *
 * Revision 7.0.1.1  86/10/16  10:54:02  lwall
 * Added Damage.  Fixed random bugs.
 *
 * Revision 7.0  86/10/08  15:14:31  lwall
 * Split into separate files.  Added amoebas and pirates.
 *
 */

#include "EXTERN.h"
#include "warp.h"
#include "object.h"
#include "sig.h"
#include "term.h"
#include "INTERN.h"
#include "util.h"

void
util_init()
{
    ;
}

void
movc3(len,src,dest)
register char *dest;
register char *src;
register int len;
{
    if (dest <= src) {
	for (; len; len--) {
	    *dest++ = *src++;
	}
    }
    else {
	dest += len;
	src += len;
	for (; len; len--) {
	    *--dest = *--src;
	}
    }
}

void
no_can_do(what)
char *what;
{
    fprintf(stderr,"Sorry, your terminal is too %s to play warp.\r\n",what);
    finalize(1);
}

int
exdis(maxnum)
int maxnum;
{
    double temp, temp2;
    double exp();
    double log();

    temp = (double) maxnum;
    temp2 = (double) myrand();
    return (int) exp(temp2 * log(temp)/0x7fff);
}

static char nomem[] = "warp: out of memory!\r\n";

/* paranoid version of malloc */

char *
safemalloc(size)
MEM_SIZE size;
{
    char *ptr;

    ptr = malloc(size?size:1);	/* malloc(0) is NASTY on our system */
    if (ptr != Nullch)
	return ptr;
    else {
	fputs(nomem,stdout);
	sig_catcher(0);
    }
    /*NOTREACHED*/
}

/* safe version of string copy */

char *
safecpy(to,from,len)
char *to;
register char *from;
register int len;
{
    register char *dest = to;

    if (from != Nullch)
	for (len--; len && (*dest++ = *from++); len--) ;
    *dest = '\0';
    return to;
}

/* copy a string up to some (non-backslashed) delimiter, if any */

char *
cpytill(to,from,delim)
register char *to;
register char *from;
register int delim;
{
    for (; *from; from++,to++) {
	if (*from == '\\' && from[1] == delim)
	    from++;
	else if (*from == delim)
	    break;
	*to = *from;
    }
    *to = '\0';
    return from;
}

/* return ptr to little string in big string, NULL if not found */

char *
instr(big, little)
char *big, *little;

{
    register char *t;
    register char *s;
    register char *x;

    for (t = big; *t; t++) {
	for (x=t,s=little; *s; x++,s++) {
	    if (!*x)
		return Nullch;
	    if (*s != *x)
		break;
	}
	if (!*s)
	    return t;
    }
    return Nullch;
}

/* effective access */

#ifdef SETUIDGID
int
eaccess(filename, mod)
char *filename;
int mod;
{
    int protection, euid;

    mod &= 7;				/* remove extraneous garbage */
    if (stat(filename, &filestat) < 0)
	return -1;
    euid = geteuid();
    if (euid == 0)
	return 0;
    protection = 7 & (filestat.st_mode >>
      (filestat.st_uid == euid ? 6 :
        (filestat.st_gid == getegid() ? 3 : 0)
      ));
    if ((mod & protection) == mod)
	return 0;
    errno = EACCES;
    return -1;
}
#endif

/* copy a string to a safe spot */

char *
savestr(str)
char *str;
{
    register char *newaddr = safemalloc((MEM_SIZE)(strlen(str)+1));

    strcpy(newaddr,str);
    return newaddr;
}

char *
getval(nam,def)
char *nam,*def;
{
    char *val;

    if ((val = getenv(nam)) == Nullch || !*val)
	val = def;
    return val;
}
