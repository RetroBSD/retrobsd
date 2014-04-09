/*
 * Mini-variant of termcap library.
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include "r.defs.h"
#include <ctype.h>

/*
 * Skip to the next field.  Notice that this is very dumb, not
 * knowing about \: escapes or any such.  If necessary, :'s can be put
 * into the termcap file in octal.
 */
static char *tskip(bp)
    register char *bp;
{
    while (*bp && *bp != ':')
        bp++;
    if (*bp == ':')
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
int tgetnum(bp, id)
    register char *bp;
    char *id;
{
    register int i, base;

    for (;;) {
        bp = tskip(bp);
        if (*bp == 0)
            return (-1);
        if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
            continue;
        if (*bp == '@')
            return(-1);
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
int tgetflag(bp, id)
    register char *bp;
    char *id;
{
    for (;;) {
        bp = tskip(bp);
        if (! *bp)
            return (0);
        if (*bp++ == id[0] && *bp != 0 && *bp++ == id[1]) {
            if (!*bp || *bp == ':')
                return (1);
            else if (*bp == '@')
                return(0);
        }
    }
}

/*
 * Tdecode does the grung work to decode the
 * string capability escapes.
 */
static char *tdecode(str, area)
    register char *str;
    char **area;
{
    register char *cp;
    register int c;
    register char *dp;
    int i,jdelay=0;
    while(*str>='0' && *str<='9') {
        jdelay=jdelay*10+(*str++ - '0');
    }
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
    if(jdelay>100) jdelay=100;
    while(jdelay--) *cp++='\200';
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
char *tgetstr(bp, id, area)
    register char *bp;
    char *id, **area;
{
    for (;;) {
        bp = tskip(bp);
        if (!*bp)
            return (0);
        if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
            continue;
        if (*bp == '@')
            return(0);
        if (*bp != '=')
            continue;
        bp++;
        return tdecode(bp, area);
    }
}

/*
 * Routine to perform cursor addressing.
 * CM is a string containing printf type escapes to allow
 * cursor addressing.  We start out ready to print the destination
 * line, and switch each time we print row or column.
 * The following escapes are defined for substituting row/column:
 *
 *  %d  as in printf
 *  %2  like %2d
 *  %3  like %3d
 *  %.  gives %c hacking special case characters
 *  %+x like %c but adding x first
 *
 *  The codes below affect the state but don't use up a value.
 *
 *  %r  reverses row/column
 *  %i  increments row/column (for one origin indexing)
 *  %%  gives %
 *
 * all other characters are ``self-inserting''.
 */
char *tgoto(CM, destcol, destline)
    char *CM;
    int destcol, destline;
{
    static char result[16];
    static char added[10];
    char *cp = CM;
    register char *dp = result;
    register int c;
    int oncol = 0;
    register int which = destline;

    added[0] = 0;
    while ((c = *cp++) != 0) {
        if (c != '%') {
            *dp++ = c;
            continue;
        }
        switch (c = *cp++) {
        case 'd':
            if (which < 10)
                goto one;
            if (which < 100)
                goto two;
            /* fall into... */
        case '3':
            *dp++ = (which / 100) | '0';
            which %= 100;
            /* fall into... */
        case '2':
two:
            *dp++ = which / 10 | '0';
one:
            *dp++ = which % 10 | '0';
swap:
            oncol = 1 - oncol;
setwhich:
            which = oncol ? destcol : destline;
            continue;

        case '+':
            which += *cp++;
            /* fall into... */
        case '.':
            *dp++ = which;
            goto swap;

        case 'r':
            oncol = 1;
            goto setwhich;

        case 'i':
            destcol++;
            destline++;
            which++;
            continue;

        case '%':
            *dp++ = c;
            continue;

        default:
            return "OOPS";
        }
    }
    strcpy(dp, added);
    return result;
}
