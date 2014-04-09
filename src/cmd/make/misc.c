#include "defs.h"
#include <string.h>
#include <stdlib.h>

FSTATIC struct nameblock *hashtab[HASHSIZE];
FSTATIC int nhashed = 0;

/*
 * simple linear hash.  hash function is sum of
 *  characters mod hash table size.
 */
int hashloc(s)
    char *s;
{
    register int i;
    register int hashval;
    register char *t;

    hashval = 0;

    for(t=s; *t!='\0' ; ++t)
        hashval += *t;

    hashval %= HASHSIZE;

    for(i=hashval;
        hashtab[i]!=0 && unequal(s,hashtab[i]->namep);
        i = (i+1)%HASHSIZE ) ;

    return(i);
}

struct nameblock *srchname(s)
    char *s;
{
    return( hashtab[hashloc(s)] );
}

int hasslash(s)
    char *s;
{
    for (; *s; ++s)
        if (*s == '/')
            return(YES);
    return(NO);
}

struct nameblock *makename(s)
    char *s;
{
    /* make a fresh copy of the string s */
    register struct nameblock *p;

    if (nhashed++ > HASHSIZE-3)
        fatal("Hash table overflow");

    p = ALLOC(nameblock);
    p->nxtnameblock = firstname;
    p->namep = copys(s);
    p->linep = 0;
    p->done = 0;
    p->septype = 0;
    p->modtime = 0;

    firstname = p;
    if (mainname == NULL)
        if (s[0]!='.' || hasslash(s))
            mainname = p;

    hashtab[hashloc(s)] = p;

    return(p);
}

char *copys(s)
    register char *s;
{
    register char *t, *t0;

    t = t0 = calloc(strlen(s) + 1, sizeof(char));
    if (t == NULL)
        fatal("out of memory");
    while ((*t++ = *s++))
        ;
    return(t0);
}

/*
 * c = concatenation of a and b
 */
char *concat(a, b, c)
    register char *a, *b;
    char *c;
{
    register char *t;
    t = c;

    while ((*t = *a++))
        t++;
    while ((*t++ = *b++));
    return c;
}

/*
 * is b the suffix of a?  if so, set p = prefix
 */
int suffix(a, b, p)
    register char *a, *b, *p;
{
    char *a0 = a, *b0 = b;

    while (*a++);
    while (*b++);

    if ((a - a0) < (b - b0))
        return 0;

    while (b > b0)
        if (*--a != *--b)
        return 0;

    while (a0 < a)
        *p++ = *a0++;
    *p = '\0';
    return 1;
}

int *ckalloc(n)
    register int n;
{
    register int *p;

    p = (int *) calloc(1,n);
    if (! p)
        fatal("out of memory");

    return p;
}

/*
 * copy string a into b, substituting for arguments
 */
char *subst(a,b)
    register char *a,*b;
{
    static int depth = 0;
    register char *s;
    char vname[BUFSIZ];
    struct varblock *vbp;
    char closer;

    if (++depth > 100)
        fatal("infinitely recursive macro?");
    if (a != 0) {
        while(*a) {
            if (*a != '$')
                *b++ = *a++;
            else if (*++a=='\0' || *a=='$')
                *b++ = *a++;
            else {
                s = vname;
                if (*a=='(' || *a=='{') {
                    closer = (*a == '(') ? ')' : '}';
                    ++a;
                    while (*a == ' ')
                        ++a;
                    while (*a!=' ' && *a!=closer && *a!='\0')
                        *s++ = *a++;
                    while (*a!=closer && *a!='\0')
                        ++a;
                    if (*a == closer)
                        ++a;
                } else
                    *s++ = *a++;

                *s = '\0';
                vbp = varptr(vname);
                if (vbp->varval != 0) {
                    b = subst(vbp->varval, b);
                    vbp->used = YES;
                }
            }
        }
    }
    *b = '\0';
    --depth;
    return(b);
}

void setvar(v, s)
    char *v, *s;
{
    struct varblock *p;

    p = varptr(v);
    if (p->noreset == 0) {
        p->varval = s;
        p->noreset = inarglist;
        if (p->used && unequal(v, "@") && unequal(v, "*")
            && unequal(v, "<") && unequal(v, "?"))
            fprintf(stderr, "Warning: %s changed after being used\n", v);
    }
}

/*
 * look for arguments with equal signs but not colons
 */
int eqsign(a)
    char *a;
{
    register char *s, *t, *b;
    char buf[256];

    while (*a == ' ')
        ++a;
    for (s=a; *s!='\0' && *s!=':'; ++s) {
        if (*s == '=') {
            b = buf;
            for (t = a; *t!='=' && *t!=' ' && *t!='\t'; t++)
                *b++ = *t;
            *b = '\0';

            for (++s; *s==' ' || *s=='\t'; ++s);
            setvar(buf, copys(s));
            return(YES);
        }
    }
    return(NO);
}

struct varblock *varptr(v)
    char *v;
{
    register struct varblock *vp;

    for (vp = firstvar; vp ; vp = vp->nxtvarblock)
        if (! unequal(v, vp->varname))
            return(vp);

    vp = ALLOC(varblock);
    vp->nxtvarblock = firstvar;
    firstvar = vp;
    vp->varname = copys(v);
    vp->varval = 0;
    return(vp);
}

void fatal1(s, t)
    char *s, *t;
{
    char buf[BUFSIZ];

    sprintf(buf, s, t);
    fatal(buf);
}

void fatal(s)
    char *s;
{
    if (s)
        fprintf(stderr, "Make: %s.  Stop.\n", s);
    else
        fprintf(stderr, "\nStop.\n");
    exit(1);
}

void yyerror(s)
    char *s;
{
    char buf[50];
    extern int yylineno;

    sprintf(buf, "line %d: %s", yylineno, s);
    fatal(buf);
}

struct chain *appendq(head, tail)
    struct chain *head;
    char *tail;
{
    register struct chain *p, *q;

    p = ALLOC(chain);
    p->datap = tail;

    if (! head)
        return(p);
    for(q = head ; q->nextp ; q = q->nextp)
        ;
    q->nextp = p;
    return(head);
}

char *mkqlist(p)
    struct chain *p;
{
    register char *qbufp, *s;
    static char qbuf[QBUFMAX];

    if (p == NULL) {
        qbuf[0] = '\0';
        return 0;
    }

    qbufp = qbuf;

    for (; p; p = p->nextp) {
        s = p->datap;
        if (qbufp+strlen(s) > &qbuf[QBUFMAX-3]) {
            fprintf(stderr, "$? list too long\n");
            break;
        }
        while (*s)
            *qbufp++ = *s++;
        *qbufp++ = ' ';
    }
    *--qbufp = '\0';
    return qbuf;
}
