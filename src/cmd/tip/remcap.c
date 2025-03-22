/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * remcap - routines for dealing with the remote host data base
 *
 * derived from termcap
 */
#include <sys/file.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef BUFSIZ
#define BUFSIZ      1024
#endif
#define MAXHOP      32              /* max number of tc= indirections */
#define SYSREMOTE   "/etc/remote"   /* system remote file */

#define tgetent     rgetent
#define tnchktc     rnchktc
#define tnamatch    rnamatch
#define tgetnum     rgetnum
#define tgetflag    rgetflag
#define tgetstr     rgetstr
#define E_TERMCAP   RM = SYSREMOTE
#define V_TERMCAP   "REMOTE"
#define V_TERM      "HOST"

char    *RM;

/*
 * termcap - routines for dealing with the terminal capability data base
 *
 * BUG:     Should use a "last" pointer in tbuf, so that searching
 *      for capabilities alphabetically would not be a n**2/2
 *      process when large numbers of capabilities are given.
 * Note:    If we add a last pointer now we will screw up the
 *      tc capability. We really should compile termcap.
 *
 * Essentially all the work here is scanning and decoding escapes
 * in string capabilities.  We don't use stdio because the editor
 * doesn't, and because living w/o it is not hard.
 */

static char *tbuf;
static int hopcount;    /* detect infinite loops in termcap, init 0 */
static char *remotefile;
static int tnchktc (void);

/*
 * Tnamatch deals with name matching.  The first field of the termcap
 * entry is a sequence of names separated by |'s, so we compare
 * against each such name.  The normal : terminator after the last
 * name (before the first field) stops us.
 */
static int tnamatch(
    char *np)
{
    register char *Np, *Bp;

    Bp = tbuf;
    if (*Bp == '#')
        return (0);
    for (;;) {
        for (Np = np; *Np && *Bp == *Np; Bp++, Np++)
            continue;
        if (*Np == 0 && (*Bp == '|' || *Bp == ':' || *Bp == 0))
            return (1);
        while (*Bp && *Bp != ':' && *Bp != '|')
            Bp++;
        if (*Bp == 0 || *Bp == ':')
            return (0);
        Bp++;
    }
}

static int getent(
    char *bp, char *name, char *cp)
{
    register int c;
    register int i = 0, cnt = 0;
    char ibuf[BUFSIZ], *cp2;
    int tf;

    tbuf = bp;
    tf = 0;
    /*
     * TERMCAP can have one of two things in it. It can be the
     * name of a file to use instead of /etc/termcap. In this
     * case it better start with a "/". Or it can be an entry to
     * use so we don't have to read the file. In this case it
     * has to already have the newlines crunched out.
     */
    if (cp && *cp) {
        if (*cp!='/') {
            cp2 = getenv(V_TERM);
            if (cp2 == (char *)0 || strcmp(name,cp2) == 0) {
                strcpy(bp,cp);
                return (tnchktc());
            } else
                tf = open(E_TERMCAP, O_RDONLY);
        } else
            tf = open(RM = cp, O_RDONLY);
    }
    if (tf == 0)
        tf = open(E_TERMCAP, O_RDONLY);
    if (tf < 0)
        return (-1);
    for (;;) {
        cp = bp;
        for (;;) {
            if (i == cnt) {
                cnt = read(tf, ibuf, BUFSIZ);
                if (cnt <= 0) {
                    close(tf);
                    return (0);
                }
                i = 0;
            }
            c = ibuf[i++];
            if (c == '\n') {
                if (cp > bp && cp[-1] == '\\') {
                    cp--;
                    continue;
                }
                break;
            }
            if (cp >= bp+BUFSIZ) {
                write(2,"Remcap entry too long\n", 23);
                break;
            } else
                *cp++ = c;
        }
        *cp = 0;

        /*
         * The real work for the match.
         */
        if (tnamatch(name)) {
            close(tf);
            return (tnchktc());
        }
    }
}

/*
 * tnchktc: check the last entry, see if it's tc=xxx. If so,
 * recursively find xxx and append that entry (minus the names)
 * to take the place of the tc=xxx entry. This allows termcap
 * entries to say "like an HP2621 but doesn't turn on the labels".
 * Note that this works because of the left to right scan.
 */
static int tnchktc()
{
    register char *p, *q;
    char tcname[16];    /* name of similar terminal */
    char tcbuf[BUFSIZ];
    char *holdtbuf = tbuf;
    int l;

    p = tbuf + strlen(tbuf) - 2;    /* before the last colon */
    while (*--p != ':')
        if (p<tbuf) {
            write(2, "Bad remcap entry\n", 18);
            return (0);
        }
    p++;
    /* p now points to beginning of last field */
    if (p[0] != 't' || p[1] != 'c')
        return (1);
    strcpy(tcname, p+3);
    q = tcname;
    while (*q && *q != ':')
        q++;
    *q = 0;
    if (++hopcount > MAXHOP) {
        write(2, "Infinite tc= loop\n", 18);
        return (0);
    }
    if (getent(tcbuf, tcname, remotefile) != 1) {
        if (strcmp(remotefile, SYSREMOTE) == 0)
            return (0);
        else if (getent(tcbuf, tcname, SYSREMOTE) != 1)
            return (0);
    }
    for (q = tcbuf; *q++ != ':'; )
        ;
    l = p - holdtbuf + strlen(q);
    if (l > BUFSIZ) {
        write(2, "Remcap entry too long\n", 23);
        q[BUFSIZ - (p-holdtbuf)] = 0;
    }
    strcpy(p, q);
    tbuf = holdtbuf;
    return (1);
}

/*
 * Get an entry for terminal name in buffer bp,
 * from the termcap file.  Parse is very rudimentary;
 * we just notice escaped newlines.
 */
int tgetent(
    char *bp, char *name)
{
    char lbuf[BUFSIZ], *cp, *p;
    int rc1, rc2;

    remotefile = cp = getenv(V_TERMCAP);
    if (cp == (char *)0 || strcmp(cp, SYSREMOTE) == 0) {
        remotefile = cp = SYSREMOTE;
        return (getent(bp, name, cp));
    } else {
        if ((rc1 = getent(bp, name, cp)) != 1)
            *bp = '\0';
        remotefile = cp = SYSREMOTE;
        rc2 = getent(lbuf, name, cp);
        if (rc1 != 1 && rc2 != 1)
            return (rc2);
        if (rc2 == 1) {
            p = lbuf;
            if (rc1 == 1)
                while (*p++ != ':')
                    ;
            if (strlen(bp) + strlen(p) > BUFSIZ) {
                write(2, "Remcap entry too long\n", 23);
                return (-1);
            }
            strcat(bp, p);
        }
        tbuf = bp;
        return (1);
    }
}

/*
 * Skip to the next field.  Notice that this is very dumb, not
 * knowing about \: escapes or any such.  If necessary, :'s can be put
 * into the termcap file in octal.
 */
static char *
tskip(
    register char *bp)
{

    while (*bp && *bp != ':')
        bp++;
    while (*bp == ':')
        bp++;
    return (bp);
}

/*
 * Return the (numeric) option id.
 * Numeric options look like
 *  li#80
 * i.e. the option string is separated from the numeric value by
 * a # character.  If the option is not found we return -1.
 * Note that we handle octal numbers beginning with 0.
 */
int tgetnum(
    char *id)
{
    register int i, base;
    register char *bp = tbuf;

    for (;;) {
        bp = tskip(bp);
        if (*bp == 0)
            return (-1);
        if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
            continue;
        if (*bp == '@')
            return (-1);
        if (*bp != '#')
            continue;
        bp++;
        base = 10;
        if (*bp == '0')
            base = 8;
        i = 0;
        while (isdigit(*bp))
            i *= base, i += *bp++ - '0';
        return (i);
    }
}

/*
 * Handle a flag option.
 * Flag options are given "naked", i.e. followed by a : or the end
 * of the buffer.  Return 1 if we find the option, or 0 if it is
 * not given.
 */
int tgetflag(
    char *id)
{
    register char *bp = tbuf;

    for (;;) {
        bp = tskip(bp);
        if (!*bp)
            return (0);
        if (*bp++ == id[0] && *bp != 0 && *bp++ == id[1]) {
            if (!*bp || *bp == ':')
                return (1);
            else if (*bp == '@')
                return (0);
        }
    }
}

/*
 * Tdecode does the grung work to decode the
 * string capability escapes.
 */
static char *
tdecode(
    register char *str,
    char **area)
{
    register char *cp;
    register int c;
    register char *dp;
    int i;

    cp = *area;
    while ((c = *str++) && c != ':') {
        switch (c) {

        case '^':
            c = *str++ & 037;
            break;

        case '\\':
            dp = "E\033^^\\\\::n\nr\rt\tb\bf\f";
            c = *str++;
nextc:
            if (*dp++ == c) {
                c = *dp++;
                break;
            }
            dp++;
            if (*dp)
                goto nextc;
            if (isdigit(c)) {
                c -= '0', i = 2;
                do
                    c <<= 3, c |= *str++ - '0';
                while (--i && isdigit(*str));
            }
            break;
        }
        *cp++ = c;
    }
    *cp++ = 0;
    str = *area;
    *area = cp;
    return (str);
}

/*
 * Get a string valued option.
 * These are given as
 *  cl=^Z
 * Much decoding is done on the strings, and the strings are
 * placed in area, which is a ref parameter which is updated.
 * No checking on area overflow.
 */
char *
tgetstr(
    char *id, char **area)
{
    register char *bp = tbuf;

    for (;;) {
        bp = tskip(bp);
        if (!*bp)
            return (0);
        if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
            continue;
        if (*bp == '@')
            return (0);
        if (*bp != '=')
            continue;
        bp++;
        return (tdecode(bp, area));
    }
}
