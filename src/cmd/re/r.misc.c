/*
 * Miscellaneous functions.
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include "r.defs.h"
#include <sys/types.h>
#include <sys/stat.h>

/*
 * Allocate zeroed memory.
 * Always succeeds.
 */
char *salloc(n)
    int n;
{
    register char *c;

    c = calloc(n, 1);
    if (! c)
        fatal(NULL);
    return (c);
}

/*
 * Check access rights.
 * Return 0 - no access;
 *        1 - only read;
 *        2 - read and write.
 */
int checkpriv(fildes)
    int fildes;
{
    register struct stat *buf;
    struct stat buffer;
    int anum, num;
    register int unum, gnum;

    if (userid == 0)
        return 2;       /* superuser accesses all */
    buf = &buffer;
    fstat(fildes, buf);
    unum = gnum = anum = 0;
    if (buf->st_uid == userid) {
        if (buf->st_mode & 0200)
            unum = 2;
        else if (buf->st_mode & 0400)
            unum = 1;
    }
    if (buf->st_gid == groupid) {
        if (buf->st_mode & 020)
            gnum = 2;
        else if (buf->st_mode & 040)
            gnum = 1;
    }
    if (buf->st_mode & 02)
        anum = 2;
    else if (buf->st_mode & 04)
        anum = 1;
    num = (unum > gnum ? unum : gnum);
    num = (num  > anum ? num  : anum);
    return num;
}

/*
 * Get file access modes.
 */
int getpriv(fildes)
    int fildes;
{
    struct stat buffer;
    register struct stat *buf;

    buf = &buffer;
    fstat(fildes,buf);
    return (buf->st_mode & 0777);
}

/*
 * Write a string to stdout.
 */
void puts1(s)
    char *s;
{
    register int len = strlen (s);

    if (len <= 0)
        return;
    if (write(1, s, len) != len)
        /* ignore errors */;
}

/*
 * Append strings.
 * Allocates memory.
 */
char *append(name, ext)
    char *name, *ext;
{
    int lname;
    register char *c, *d, *newname;

    lname = 0;
    c = name;
    while (*c++)
        ++lname;
    c = ext;
    while (*c++)
        ++lname;
    newname = c = salloc(lname + 1);
    d = name;
    while (*d)
        *c++ = *d++;
    d = ext;
    while ((*c++ = *d++) != 0);
    return newname;
}

/*
 * Convert a string to number.  Store result into *i.
 * Return pointer to first symbol after a number, or 0 if none.
 */
char *s2i(s, i)
    char *s;
    int *i;
{
    register char lc, c;
    register int val;
    int sign;
    char *ans;

    sign = 1;
    val = lc = 0;
    ans = 0;
    while ((c = *s++) != 0) {
        if (c >= '0' && c <= '9') val = 10*val + c - '0';
        else if (c == '-' && lc == 0) sign = -1;
        else {
            ans = --s;
            break;
        }
        lc = c;
    }
    *i = val * sign;
    return ans;
}

/*
 * Get user id as printable text.
 */
char *getnm(uid)
    int uid;
{
#define LNAME 8
    static char namewd[LNAME+1];
    register int i;

    i = LNAME;
    namewd[LNAME]=0;
    while (i > 1 && uid > 0) {
            namewd[--i] = '0' + uid %10;
            uid /= 10;
    }
    return &namewd[i];
}

/*
 * Read word.
 */
int get1w(fd)
    int fd;
{
    int i;

    if (read(fd, &i, sizeof(int)) != sizeof(int))
        return -1;
    return i;
}

/*
 * Read byte.
 */
int get1c(fd)
    int fd;
{
    char c;

    if (read(fd, &c, 1) == 1)
        return (unsigned char) c;
    return -1;
}

/*
 * Write word.
 */
void put1w(w, fd)
    int fd, w;
{
    if (write(fd, &w, sizeof(int)) != sizeof(int))
        /* ignore errors */;
}

/*
 * Write byte.
 */
void put1c(c, fd)
    int c, fd;
{
    char sym = c;

    if (write(fd, &sym, 1) != 1)
        /* ignore errors */;
}
