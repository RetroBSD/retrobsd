/*
 * test expression
 * [ expression ]
 */
#include <sys/stat.h>
#include <sys/types.h>

#include "defs.h"

int ap, ac;
char **av;

int exp0(void);
int e1(void);
int e2(void);
int e3(void);
int tio(char *a, int f);
int filtyp(char *f, int field);
int ftype(char *f, int field);
int fsizep(char *f);
void bfailed(char *s1, char *s2, char *s3);

int test(int argn, char *com[])
{
    ac = argn;
    av = com;
    ap = 1;
    if (eq(com[0], "[")) {
        if (!eq(com[--ac], "]"))
            failed("test", "] missing");
    }
    com[ac] = NIL;
    if (ac <= 1)
        return (1);
    return (exp0() ? 0 : 1);
}

char *nxtarg(int mt)
{
    if (ap >= ac) {
        if (mt) {
            ap++;
            return (NIL);
        }
        failed("test", "argument expected");
    }
    return (av[ap++]);
}

int exp0()
{
    int p1;
    char *p2;

    p1 = e1();
    p2 = nxtarg(1);
    if (p2 != NIL) {
        if (eq(p2, "-o"))
            return (p1 | exp0());

        if (eq(p2, "]") && !eq(p2, ")"))
            failed("test", synmsg);
    }
    ap--;
    return (p1);
}

int e1()
{
    int p1;
    char *p2;

    p1 = e2();
    p2 = nxtarg(1);

    if ((p2 != NIL) && eq(p2, "-a"))
        return (p1 & e1());
    ap--;
    return (p1);
}

int e2()
{
    if (eq(nxtarg(0), "!"))
        return (!e3());
    ap--;
    return (e3());
}

int e3()
{
    int p1;
    register char *a;
    char *p2;
    long atol();
    long int1, int2;

    a = nxtarg(0);
    if (eq(a, "(")) {
        p1 = exp0();
        if (!eq(nxtarg(0), ")"))
            failed("test", ") expected");
        return (p1);
    }
    p2 = nxtarg(1);
    ap--;
    if ((p2 == NIL) || (!eq(p2, "=") && !eq(p2, "!="))) {
        if (eq(a, "-r"))
            return (tio(nxtarg(0), 4));
        if (eq(a, "-w"))
            return (tio(nxtarg(0), 2));
        if (eq(a, "-x"))
            return (tio(nxtarg(0), 1));
        if (eq(a, "-d"))
            return (filtyp(nxtarg(0), S_IFDIR));
        if (eq(a, "-c"))
            return (filtyp(nxtarg(0), S_IFCHR));
        if (eq(a, "-b"))
            return (filtyp(nxtarg(0), S_IFBLK));
        if (eq(a, "-f"))
            return (filtyp(nxtarg(0), S_IFREG));
        if (eq(a, "-u"))
            return (ftype(nxtarg(0), S_ISUID));
        if (eq(a, "-g"))
            return (ftype(nxtarg(0), S_ISGID));
        if (eq(a, "-k"))
            return (ftype(nxtarg(0), S_ISVTX));
        if (eq(a, "-p"))
            return (filtyp(nxtarg(0), S_IFIFO));
        if (eq(a, "-s"))
            return (fsizep(nxtarg(0)));
        if (eq(a, "-t")) {
            if (ap >= ac) /* no args */
                return (isatty(1));
            else if (eq((a = nxtarg(0)), "-a") || eq(a, "-o")) {
                ap--;
                return (isatty(1));
            } else
                return (isatty(atoi(a)));
        }
        if (eq(a, "-n"))
            return (!eq(nxtarg(0), ""));
        if (eq(a, "-z"))
            return (eq(nxtarg(0), ""));
    }

    p2 = nxtarg(1);
    if (p2 == NIL)
        return (!eq(a, ""));
    if (eq(p2, "-a") || eq(p2, "-o")) {
        ap--;
        return (!eq(a, ""));
    }
    if (eq(p2, "="))
        return (eq(nxtarg(0), a));
    if (eq(p2, "!="))
        return (!eq(nxtarg(0), a));
    int1 = atol(a);
    int2 = atol(nxtarg(0));
    if (eq(p2, "-eq"))
        return (int1 == int2);
    if (eq(p2, "-ne"))
        return (int1 != int2);
    if (eq(p2, "-gt"))
        return (int1 > int2);
    if (eq(p2, "-lt"))
        return (int1 < int2);
    if (eq(p2, "-ge"))
        return (int1 >= int2);
    if (eq(p2, "-le"))
        return (int1 <= int2);

    bfailed(btest, badop, p2);
    /* NOTREACHED */
    return 0;
}

int tio(char *a, int f)
{
    if (access(a, f) == 0)
        return (1);
    else
        return (0);
}

int ftype(char *f, int field)
{
    struct stat statb;

    if (stat(f, &statb) < 0)
        return (0);
    if ((statb.st_mode & field) == field)
        return (1);
    return (0);
}

int filtyp(char *f, int field)
{
    struct stat statb;

    if (stat(f, &statb) < 0)
        return (0);
    if ((statb.st_mode & S_IFMT) == field)
        return (1);
    else
        return (0);
}

int fsizep(char *f)
{
    struct stat statb;

    if (stat(f, &statb) < 0)
        return (0);
    return (statb.st_size > 0L);
}

/*
 * fake diagnostics to continue to look like original
 * test(1) diagnostics
 */
void bfailed(char *s1, char *s2, char *s3)
{
    prp();
    prs(s1);
    if (s2) {
        prs(colon);
        prs(s2);
        prs(s3);
    }
    newline();
    exitsh(ERROR);
}
