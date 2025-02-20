/*
 * BASIC PROCEDURE.  RECURSIVE.
 *
 * p->done = 0   don't know what to do yet
 * p->done = 1   file in process of being updated
 * p->done = 2   file already exists in current state
 * p->done = 3   file make failed
 */
#include <signal.h>
#include <stdlib.h>
#include <strings.h>

#include "defs.h"

extern char *sys_siglist[], arfile[], *arfname;

int docom1(char *comstring, int nohalt, int noprint)
{
    register int status;

    if (comstring[0] == '\0')
        return (0);

    if (!silflag && (!noprint || noexflag)) {
        printf("%s%s\n", (noexflag ? "" : prompt), comstring);
        fflush(stdout);
    }
    if (noexflag)
        return (0);

    status = dosys(comstring, nohalt);
    if (status != 0) {
        unsigned sig = status & 0177;
        if (sig) {
            if (sig < NSIG && sys_siglist[sig] != NULL && *sys_siglist[sig] != '\0')
                printf("*** %s", sys_siglist[sig]);
            else
                printf("*** Signal %d", sig);
            if (status & 0200)
                printf(" - core dumped");
        } else
            printf("*** Exit %d", (status >> 8) & 0xff);

        if (nohalt)
            printf(" (ignored)\n");
        else
            printf("\n");
        fflush(stdout);
    }
    return (status);
}

int docom(struct shblock *q)
{
    register char *s;
    register int ign, nopr;
    char string[OUTMAX];
    char string2[OUTMAX];

    ++ndocoms;
    if (questflag)
        return (NO);

    if (touchflag) {
        s = varptr("@")->varval;
        if (!silflag)
            printf("touch(%s)\n", s);
        if (!noexflag)
            touch(YES, s);
    } else {
        for (; q; q = q->nxtshblock) {
            subst(q->shbp, string2);
            fixname(string2, string);

            ign = ignerr;
            nopr = NO;
            for (s = string; *s == '-' || *s == '@'; ++s) {
                if (*s == '-')
                    ign = YES;
                else
                    nopr = YES;
            }

            if (docom1(s, ign, nopr) && !ign) {
                if (keepgoing)
                    return (YES);
                else
                    fatal((char *)NULL);
            }
        }
    }
    return (NO);
}

int doname(struct nameblock *p, int reclevel, TIMETYPE *tval)
{
    int errstat;
    u_char okdel1;
    u_char didwork;
    TIMETYPE td, td1, tdep, ptime, ptime1;
    register struct depblock *q;
    struct depblock *qtemp, *suffp, *suffp1;
    struct nameblock *p1, *p2;
    struct shblock *implcom, *explcom;
    register struct lineblock *lp;
    struct lineblock *lp1, *lp2;
    char *pnamep, *p1namep, *cp, *savenamep = NULL;
    struct chain *qchain;
    /*
     * temp and sourcename are mutually exclusive - save 254 bytes of stack by
     * reusing sourcename's buffer
     */
    char sourcename[256], prefix[256], *temp = sourcename, concsuff[20];

    {
        /*
         * VPATH= ${PATH1}:${PATH2} didn't work.  This fix is so ugly I don't
         * even want to think about it.  Basically it grabs VPATH and
         * explicitly does macro expansion before resolving names.  Why
         * VPATH didn't get handled correctly I have no idea; the symptom
         * was that, while macro expansion got done, the .c files in the
         * non-local directories wouldn't be found.
         */
        static struct varblock *vpath_cp;
        static char vpath_exp[256];

        if (!vpath_cp) {
            vpath_cp = varptr("VPATH");
            if (vpath_cp->varval) {
                subst(vpath_cp->varval, vpath_exp);
                setvar("VPATH", vpath_exp);
            }
        }
    }

    if (p == 0) {
        *tval = 0;
        return (0);
    }

    if (dbgflag) {
        printf("doname(%s,%d)\n", p->namep, reclevel);
        fflush(stdout);
    }

    if (p->done > 0) {
        *tval = p->modtime;
        return (p->done == 3);
    }

    errstat = 0;
    tdep = 0;
    implcom = 0;
    explcom = 0;
    ptime = exists(p);
    ptime1 = 0;
    didwork = NO;
    p->done = 1; /* avoid infinite loops */

    qchain = NULL;

    /* Expand any names that have embedded metacharaters */

    for (lp = p->linep; lp; lp = lp->nxtlineblock) {
        for (q = lp->depp; q; q = qtemp) {
            qtemp = q->nxtdepblock;
            expand(q);
        }
    }

    /* make sure all dependents are up to date */

    for (lp = p->linep; lp; lp = lp->nxtlineblock) {
        td = 0;
        for (q = lp->depp; q; q = q->nxtdepblock) {
            errstat += doname(q->depname, reclevel + 1, &td1);
            if (dbgflag)
                printf("TIME(%s)=%ld\n", q->depname->namep, td1);
            if (td1 > td)
                td = td1;
            if (ptime < td1)
                qchain = appendq(qchain, q->depname->namep);
        }
        if (p->septype == SOMEDEPS) {
            if (lp->shp != 0) {
                if (ptime < td || (ptime == 0 && td == 0) || lp->depp == 0) {
                    okdel1 = okdel;
                    okdel = NO;
                    setvar("@", p->namep);
                    setvar("?", mkqlist(qchain));
                    qchain = NULL;
                    if (!questflag)
                        errstat += docom(lp->shp);
                    setvar("@", (char *)NULL);
                    okdel = okdel1;
                    ptime1 = prestime();
                    didwork = YES;
                }
            }
        } else {
            if (lp->shp != 0) {
                if (explcom)
                    fprintf(stderr, "Too many command lines for `%s'\n", p->namep);
                else
                    explcom = lp->shp;
            }

            if (td > tdep)
                tdep = td;
        }
    }

    /* Look for implicit dependents, using suffix rules */

    if (index(p->namep, '(')) {
        savenamep = p->namep;
        p->namep = arfname;
    }

    for (lp = sufflist; lp; lp = lp->nxtlineblock) {
        for (suffp = lp->depp; suffp; suffp = suffp->nxtdepblock) {
            pnamep = suffp->depname->namep;
            if (suffix(p->namep, pnamep, prefix)) {
                if (savenamep)
                    pnamep = ".a";

                srchdir(concat(prefix, "*", temp), NO, (struct depblock *)NULL);
                for (lp1 = sufflist; lp1; lp1 = lp1->nxtlineblock) {
                    for (suffp1 = lp1->depp; suffp1; suffp1 = suffp1->nxtdepblock) {
                        p1namep = suffp1->depname->namep;
                        if ((p1 = srchname(concat(p1namep, pnamep, concsuff))) &&
                            (p2 = srchname(concat(prefix, p1namep, sourcename)))) {
                            errstat += doname(p2, reclevel + 1, &td);
                            if (ptime < td)
                                qchain = appendq(qchain, p2->namep);
                            if (dbgflag)
                                printf("TIME(%s)=%ld\n", p2->namep, td);
                            if (td > tdep)
                                tdep = td;
                            setvar("*", prefix);
                            if (p2->alias)
                                setvar("<", copys(p2->alias));
                            else
                                setvar("<", copys(p2->namep));
                            for (lp2 = p1->linep; lp2; lp2 = lp2->nxtlineblock) {
                                implcom = lp2->shp;
                                if (implcom)
                                    break;
                            }
                            goto endloop;
                        }
                    }
                }
                cp = rindex(prefix, '/');
                if (cp++ == 0)
                    cp = prefix;
                setvar("*", cp);
            }
        }
    }
endloop:
    if (savenamep)
        p->namep = savenamep;

    if (errstat == 0 && (ptime < tdep || (ptime == 0 && tdep == 0))) {
        ptime = (tdep > 0) ? tdep : prestime();
        setvar("@", savenamep ? arfile : p->namep);
        setvar("?", mkqlist(qchain));
        if (explcom) {
            errstat += docom(explcom);
        } else if (implcom) {
            errstat += docom(implcom);
        } else if (p->septype == 0) {
            p1 = srchname(".DEFAULT");
            if (p1) {
                if (p->alias)
                    setvar("<", p->alias);
                else
                    setvar("<", p->namep);
                for (lp2 = p1->linep; lp2; lp2 = lp2->nxtlineblock) {
                    implcom = lp2->shp;
                    if (implcom) {
                        errstat += docom(implcom);
                        break;
                    }
                }
            } else if (keepgoing) {
                printf("Don't know how to make %s\n", p->namep);
                ++errstat;
            } else
                fatal1(" Don't know how to make %s", p->namep);
        }
        setvar("@", (char *)NULL);
        if (noexflag || (ptime = exists(p)) == 0)
            ptime = prestime();
    } else if (errstat != 0 && reclevel == 0)
        printf("`%s' not remade because of errors\n", p->namep);

    else if (!questflag && reclevel == 0 && didwork == NO)
        printf("`%s' is up to date.\n", p->namep);

    if (questflag && reclevel == 0)
        exit(ndocoms > 0 ? -1 : 0);

    p->done = (errstat ? 3 : 2);
    if (ptime1 > ptime)
        ptime = ptime1;
    p->modtime = ptime;
    *tval = ptime;
    return (errstat);
}

/*
 * If there are any Shell meta characters in the name,
 * expand into a list, after searching directory
 */
void expand(struct depblock *q)
{
    register char *s;
    char *s1;
    struct depblock *p;

    if (q->depname == NULL)
        return;
    s1 = q->depname->namep;
    for (s = s1;;) {
        switch (*s++) {
        case '\0':
            return;

        case '*':
        case '?':
        case '[':
            p = srchdir(s1, YES, q->nxtdepblock);
            if (p) {
                q->nxtdepblock = p;
                q->depname = 0;
            }
            return;
        }
    }
}
