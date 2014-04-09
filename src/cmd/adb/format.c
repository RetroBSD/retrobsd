#include "defs.h"
#include <sys/wait.h>
#ifdef CROSS
#   include </usr/include/ctype.h>
#else
#   include <ctype.h>
#endif

void
scanform(icount, ifp, itype, ptype)
    long    icount;
    char    *ifp;
{
    char    *fp;
    char    modifier;
    int     fcount, init=1;
    long    savdot;

    while (icount) {
        fp = ifp;
        if (init == 0 && findsym(shorten(dot), ptype) == 0 && maxoff) {
            print("\n%s:%16t", cache_sym(symbol));
        }
        savdot = dot;
        init = 0;

        /* now loop over format */
        while (*fp && errflg == 0) {
            modifier = *fp;
            if (isdigit(modifier)) {
                fcount = 0;
                modifier = *fp++;
                while (isdigit(modifier)) {
                    fcount *= 10;
                    fcount += modifier-'0';
                }
                fp--;
            } else {
                fcount = 1;
            }

            if (*fp == 0)
                break;
            fp = exform(fcount, fp, itype, ptype);
        }
        dotinc = dot - savdot;
        dot = savdot;

        if (errflg) {
            if (icount < 0) {
                errflg = 0;
                break;
            } else {
                error(errflg);
            }
        }
        if (--icount) {
            dot = inkdot(dotinc);
        }
        if (mkfault) {
            error((char *)0);
        }
    }
}

static void
printesc(c)
{
    c &= STRIP;
    if (c < SP || c > '~' || c == '@') {
        print("@%c", (c == '@') ? '@' : c ^ 0140);
    } else {
        printc(c);
    }
}

char *
exform(fcount, ifp, itype, ptype)
    int     fcount;
    char    *ifp;
    int     itype, ptype;
{
    /* execute single format item `fcount' times
     * sets `dotinc' and moves `dot'
     * returns address of next format item
     */
    u_int   w;
    long    savdot, wx;
    char    *fp = 0;
    int     c, modifier;
    struct {
        long    sa;
        int     sb, sc;
    } fw;

    while (fcount > 0) {
        fp = ifp;
        c = *fp;
        //int longpr = (c >= 'A' && c <= 'Z') || (c == 'f');
        if (itype == NSP || *fp == 'a') {
            wx = dot;
            w = dot;
        } else {
            w = get(dot, itype);
            //if (longpr)
            //    wx = itol(w, get(inkdot(2), itype));
            //else
            wx = w;
        }
        if (c == 'F') {
            fw.sb = get(inkdot(4), itype);
            fw.sc = get(inkdot(6), itype);
        }
        if (errflg)
            return(fp);
        if (mkfault)
            error((char *)0);
        var[0] = wx;
        modifier = *fp++;
        dotinc = 4;

        if (! (printptr - printbuf) && modifier != 'a')
            print("%16m");

        switch(modifier) {

        case SP: case TB:
            break;

        case 't': case 'T':
            print("%T", fcount);
            return fp;

        case 'r': case 'R':
            print("%M", fcount);
            return fp;

        case 'a':
            psymoff(dot, ptype, ":%16t");
            dotinc = 0;
            break;

        case 'p':
            psymoff(var[0], ptype, "%16t");
            break;

        case 'u':
            print("%-8u", w);
            break;

        case 'U':
            print("%-16U", wx);
            break;

        case 'c': case 'C':
            if (modifier == 'C') {
                printesc(w & LOBYTE);
            } else {
                printc(w & LOBYTE);
            }
            dotinc = 1;
            break;

        case 'b': case 'B':
            print("%-4x", w & LOBYTE);
            dotinc = 1;
            break;

        case 's': case 'S':
            savdot = dot;
            dotinc = 1;
            while ((c = get(dot, itype) & LOBYTE) && errflg == 0) {
                dot = inkdot(1);
                if (modifier == 'S') {
                    printesc(c);
                } else {
                    printc(c);
                }
                endline();
            }
            dotinc = dot - savdot + 1;
            dot = savdot;
            break;

        case 'x':
            print("%-8x", w);
            break;

        case 'X':
            print("%-16X", wx);
            break;

        case 'Y':
            print("%-24Y", wx);
            break;

        case 'q':
            print("%-8q", w);
            break;

        case 'Q':
            print("%-16Q", wx);
            break;

        case 'o':
        case 'w':
            print("%-8o", w);
            break;

        case 'O':
        case 'W':
            print("%-16O", wx);
            break;

        case 'i':
            printins(itype, dot, w); printc(EOR);
            break;

        case 'd':
            print("%-8d", w);
            break;

        case 'D':
            print("%-16D", wx);
            break;

        case 'f':
            *(double *)&fw = 0.0;
            fw.sa = wx;
            print("%-16.9f", *(double *)&fw);
            dotinc = 4;
            break;

        case 'F':
            fw.sa = wx;
            print("%-32.18F", *(double *)&fw);
            dotinc = 8;
            break;

        case 'n': case 'N':
            printc('\n');
            dotinc = 0;
            break;

        case '"':
            dotinc = 0;
            while (*fp != '"' && *fp) {
                printc(*fp++);
            }
            if (*fp) {
                fp++;
            }
            break;

        case '^':
            dot = inkdot(-dotinc * fcount);
            return fp;

        case '+':
            dot = inkdot(fcount);
            return fp;

        case '-':
            dot = inkdot(-fcount);
            return fp;

        default:
            error(BADMOD);
        }
        if (itype != NSP) {
            dot = inkdot(dotinc);
        }
        fcount--;
        endline();
    }
    return fp;
}

void
unox()
{
    int     rc, status, unixpid;
    char    *argp = lp;

    while (lastc != EOR) {
        rdc();
    }
    if ((unixpid = fork()) == 0) {
        signal(SIGINT, sigint);
        signal(SIGQUIT, sigqit);
        *lp = 0;
        execl("/bin/sh", "sh", "-c", argp, (char*)0);
        exit(16);
    } else if (unixpid == -1) {
        error(NOFORK);
    } else {
        signal(SIGINT, SIG_IGN);
        while ((rc = wait(&status)) != unixpid && rc != -1);
        signal(SIGINT, sigint);
        printc('!');
        lp--;
    }
}

long
inkdot(incr)
{
    long    newdot;

    newdot = dot + incr;
    if ((dot ^ newdot) >> 24) {
        error(ADWRAP);
    }
    return newdot;
}
