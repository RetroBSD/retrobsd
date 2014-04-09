/*
 * Portable Forth interpreter.
 * Copyright (C) 1990-2012 Serge Vakulenko, <serge@vak.ru>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include "io.h"
#include "forth.h"

#define OKPROMPT        "Ok\n> "
#define ERRPROMPT       "> "
#define COMPILEPROMPT   ": "

value_t     defbuf [DEFSZ];
value_t     *defptr;

value_t     *execstack [NESTSZ];
value_t     *execptr;
int         execcnt;

struct loop loopstack [LOOPSZ];
struct loop *loopptr;

union memory memory;
integer_t   here = 0;
integer_t   lowmem;

integer_t   stack [STACKSZ];
integer_t   *stackptr;

jmp_buf     errjmp;

ident_t     dict [DICTSZ];
ident_t     *dictptr;
ident_t     *lowdictptr;

int         outstack [OUTSZ];
int         outptr;
int         instack [INSZ];
int         inptr;

integer_t   compindex;
integer_t   baseindex;
integer_t   spanindex;
integer_t   hldindex;

int         tty;
int         debug;
int         boln;
int         errorflag;

value_t     _x;

void forthresume (int clrflag)
{
    errorflag = 1;
    if (clrflag)
        stackptr = stack;
    longjmp (errjmp, 1);
}

void error (char *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    vprintf (fmt, ap);
    va_end (ap);
    printf ("\n");
    forthresume (1);
}

void usage ()
{
    printf ("Usage: forth [-d] [files...]\n");
    exit (1);
}

void initdict ()
{
    struct table *t;
    ident_t *w;

    dictptr = dict;
    for (t=func_table; t->name; ++t, ++dictptr) {
        strncpy (dictptr->name, t->name, NAMESZ);
        dictptr->name [NAMESZ-1] = 0;
        dictptr->type = t->type;
        dictptr->val.p = t->func;
        dictptr->immed = t->immed;
    }

    w = enter ("bl");
    ++dictptr;
    w->type = FGENINT;
    w->val.i = ' ';

    w = enter ("pad");
    ++dictptr;
    w->type = FGENINT;
    w->val.i = pad;

    lowmem = here;

    compindex = alloc ((integer_t) 1);
    w = enter ("state");
    ++dictptr;
    w->type = FGENINT;
    w->val.i = compindex;

    baseindex = alloc ((integer_t) 1);
    w = enter ("base");
    ++dictptr;
    w->type = FGENINT;
    w->val.i = baseindex;

    spanindex = alloc ((integer_t) 1);
    w = enter ("span");
    ++dictptr;
    w->type = FGENINT;
    w->val.i = spanindex;

    hldindex = alloc ((integer_t) 1);
    w = enter ("hld");
    ++dictptr;
    w->type = FGENINT;
    w->val.i = hldindex;

    lowdictptr = dictptr;
}

void scompile (ident_t *w)
{
    (defptr++)->i = w->type;
    switch (w->type) {
    case FGENINT:
    case FGENFLT:
    case FSUBR:
    case FHARDWARE:
        *defptr++ = w->val;
        break;
    }
}

void sexecute (ident_t *w)
{
    if (w >= dictptr)
        error ("bad execute");
    switch (w->type) {
    case FGENINT:
        push (w->val.i);
        break;
    case FGENFLT:
        fpush (w->val.r);
        break;
    case FSUBR:
        execute (w->val.v);
        break;
    case FHARDWARE:
        (*w->val.p) ();
        break;
    default:
        error ("%s ?", w->name);
    }
}

int valsize (value_t *a)
{
    switch (a->i) {
    default:        return (1);
    case FGENINT:
    case FGENFLT:
    case FSUBR:
    case FHARDWARE: return (2);
    case FSTRING:
    case FCSTRING:
    case FQUOTE:    return (3 + a[1].i / sizeof (value_t));
    }
}

void run ()
{
    int count;
    integer_t c;
    char *p;

    while (execptr) {
        switch ((execptr++)->i) {
        case FGENINT:
            push ((execptr++)->i);
            continue;
        case FGENFLT:
            fpush ((execptr++)->r);
            continue;
        case FSUBR:
            execute ((execptr++)->v);
            continue;
        case FHARDWARE:
            (*(execptr++)->p) ();
            continue;
        case FSTRING:
        case FCSTRING:
            c = (execptr++)->i;
            p = (char*) execptr++;
            execptr += c / sizeof (value_t);
            for (; *p; ++p)
                putc (*p, stdout);
            break;
        case FQUOTE:
            c = (execptr++)->i;
            p = (char*) execptr++;
            execptr += c / sizeof (value_t);
            push (c = here);
            while (*p)
                STOREBYTE (c++, *p++);
            STOREBYTE (c, 0);
            break;
        case FCONDIF:
            if (pop ())
                continue;
            for (count=0; execptr->i; execptr+=valsize(execptr))
                if (execptr->i == FCONDIF)
                    ++count;
                else if (execptr->i == FCONDTHEN) {
                    if (--count < 0) {
                        ++execptr;
                        break;
                    }
                } else if (execptr->i == FCONDELSE)
                    if (count <= 0) {
                        ++execptr;
                        break;
                    }
            continue;
        case FCONDELSE:
            for (count=0; execptr->i; execptr+=valsize(execptr))
                if (execptr->i == FCONDIF)
                    ++count;
                else if (execptr->i == FCONDTHEN)
                    if (--count < 0) {
                        ++execptr;
                        break;
                    }
            continue;
        case FUNTIL:
            if (loopptr <= loopstack)
                error ("until but no begin");
            if (loopptr[-1].type != FBEGIN)
                error ("bad until");
            if (pop ()) {
                --loopptr;
                continue;
            }
            execptr = loopptr[-1].ptr;
            continue;
        case FREPEAT:
            if (loopptr <= loopstack)
                error ("repeat but no begin");
            if (loopptr[-1].type != FBEGIN)
                error ("bad repeat");
            execptr = loopptr[-1].ptr;
            continue;
        case FWHILE:
            if (pop ())
                continue;
            for (count=0; execptr->i; execptr+=valsize(execptr))
                if (execptr->i == FBEGIN)
                    ++count;
                else if (execptr->i==FUNTIL || execptr->i==FREPEAT)
                    if (--count < 0) {
                        ++execptr;
                        break;
                    }
            continue;
        case FLEAVE:
            if (loopptr <= loopstack)
                error ("bad leave");
leave:                  for (count=0; execptr->i; execptr+=valsize (execptr))
                if (execptr->i==FBEGIN || execptr->i==FDO || execptr->i==FIFDO)
                    ++count;
                else if (execptr->i==FUNTIL || execptr->i==FREPEAT ||
                    execptr->i==FLOOP || execptr->i==FPLOOP)
                    if (--count < 0) {
                        ++execptr;
                        break;
                    }
            --loopptr;
            continue;
        case FDO:
            if (loopptr >= loopstack+LOOPSZ)
                error ("too deep do nesting");
            loopptr->cnt = loopptr->low = pop ();
            loopptr->up = pop ();
            loopptr->type = FDO;
            loopptr->ptr = execptr;
            ++loopptr;
            continue;
        case FIFDO:
            if (loopptr >= loopstack+LOOPSZ)
                error ("too deep ?do nesting");
            loopptr->cnt = loopptr->low = pop ();
            loopptr->up = pop ();
            loopptr->type = FIFDO;
            loopptr->ptr = execptr;
            ++loopptr;
            if (loopptr[-1].low == loopptr[-1].up)
                goto leave;
            continue;
        case FLOOP:
            if (loopptr <= loopstack)
                error ("bad loop");
            if (loopptr[-1].type != FDO && loopptr[-1].type != FIFDO)
                error ("invalid loop");
            ++loopptr[-1].cnt;
            if (loopptr[-1].cnt >= loopptr[-1].up) {
                --loopptr;
                continue;
            }
            execptr = loopptr[-1].ptr;
            continue;
        case FPLOOP:
            if (loopptr <= loopstack)
                error ("bad +loop");
            if (loopptr[-1].type != FDO && loopptr[-1].type != FIFDO)
                error ("invalid loop");
            c = pop ();
            if ((c>0 && loopptr[-1].cnt<loopptr[-1].up &&
                loopptr[-1].cnt+c>=loopptr[-1].up) ||
                (c<0 && loopptr[-1].cnt>=loopptr[-1].up &&
                loopptr[-1].cnt+c<loopptr[-1].up))
            {
                --loopptr;
                continue;
            }
            loopptr[-1].cnt += c;
            execptr = loopptr[-1].ptr;
            continue;
        case FCOUNTI:
            if (loopptr <= loopstack)
                error ("bad i");
            push (loopptr[-1].cnt);
            continue;
        case FCOUNTJ:
            if (loopptr <= loopstack+1)
                error ("bad j");
            push (loopptr[-2].cnt);
            continue;
        case FCOUNTK:
            if (loopptr <= loopstack+2)
                error ("bad k");
            push (loopptr[-3].cnt);
            continue;
        case FBEGIN:
            if (loopptr >= loopstack+LOOPSZ)
                error ("too deep begin nesting");
            loopptr->type = FBEGIN;
            loopptr->ptr = execptr;
            ++loopptr;
            continue;
        case FCONDTHEN:
            continue;
        case FEXIT:
        case 0:
doexit:         --execcnt;
            if (execcnt < 0)
                return;
            execptr = execstack [execcnt];
            continue;
        }
    }
    goto doexit;
}

int symdigit (int c)
{
    c &= 0377;
    switch (base) {
    case BINARY:
        switch (c) {
        case '0': return (0);
        case '1': return (1);
        }
        return (-1);
    case OCTAL:
        switch (c) {
        case '0': case '1': case '2': case '3': case '4': case '5':
        case '6': case '7':
            return (c - '0');
        }
        return (-1);
    case DECIMAL:
        switch (c) {
        case '0': case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9':
            return (c - '0');
        }
        return (-1);
    case HEX:
        switch (c) {
        case '0': case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9':
            return (c - '0');
        }
        switch (c) {
        case 'a': case 'A': return (10);
        case 'b': case 'B': return (11);
        case 'c': case 'C': return (12);
        case 'd': case 'D': return (13);
        case 'e': case 'E': return (14);
        case 'f': case 'F': return (15);
        }
        return (-1);
    }
    return (-1);
}

int integ (char *s, integer_t *i)
{
    integer_t c;
    int neg, d;

    neg = (*s == '-');
    if (neg)
        ++s;
    if (! *s)
        return (0);
    c = 0;
    while ((d = symdigit (*s)) >= 0) {
        c = c*base + d;
        ++s;
    }
    if (*s)
        return (0);
    *i = neg ? -c : c;
    return (1);
}

int realnum (char *s, real_t *r)
{
    int d, e;
    int neg = 0, eneg = 0, ep = 0;
    real_t v;

    if (*s == '-') {
        neg = 1;
        ++s;
    }
    if (! *s)
        return (0);
    v = 0;
    while ((d = symdigit (*s)) >= 0) {
        v = v*base + d;
        ++s;
    }
    if (*s=='.' || *s==',') {
        ++s;
        while ((d = symdigit (*s)) >= 0) {
            v = v*base + d;
            ++s;
            ++ep;
        }
    }
    if (neg)
        v = -v;
    e = 0;
    if (*s=='e' || *s=='E') {
        ++s;
        if (*s == '-') {
            eneg = 1;
            ++s;
        }
        while ((d = symdigit (*s)) >= 0) {
            e = e*base + d;
            ++s;
        }
    }
    if (*s)
        return (0);
    if (eneg)
        e = -e;
    e -= ep;
    if (e < 0) {
        while (++e <= 0)
            v /= base;
    } else {
        while (--e >= 0)
            v *= base;
    }
    *r = v;
    return (1);
}

void forth ()
{
    char *p;
    ident_t *w;
    integer_t i;
    real_t r;

    tty = isatty (fileno (stdin));
    boln = 1;
    compilation = 0;
    base = DECIMAL;
    errorflag = 0;
    stackptr = stack;
    setjmp (errjmp);
    loopptr = loopstack;
    defptr = defbuf;
    execcnt = -1;
    if (inptr) {
        while (--inptr > 0)
            close (instack [inptr]);
        fclose (stdin);
        fdopen (instack [0], "r");
    }
    if (outptr) {
        while (--outptr > 0)
            close (outstack [inptr]);
        fclose (stdout);
        fdopen (outstack [0], "a");
    }
    for (;;) {
        if (! (p = getword (' ', 1)))
            return;
        errorflag = 0;
        w = find (p);
        if (w) {
            if (compilation && ! w->immed) {
                scompile (w);
                continue;
            }
            sexecute (w);
            if (execcnt >= 0)
                run ();
            continue;
        }
        if (integ (p, &i)) {
            if (compilation) {
                (defptr++)->i = FGENINT;
                (defptr++)->i = i;
                continue;
            }
            push (i);
            continue;
        }
        if (realnum (p, &r)) {
            if (compilation) {
                (defptr++)->i = FGENFLT;
                (defptr++)->r = r;
                continue;
            }
            fpush (r);
            continue;
        }
        error ("%s ?", p);
    }
}

int main (int argc, char **argv)
{
    int fdin;

    printf ("Portable Forth, Version 1.4\n");
    printf ("Copyright (C) 1990-2012 Serge Vakulenko\n\n");
    for (++argv, --argc; argc>0 && **argv=='-'; ++argv, --argc) {
        if (! strcmp ("-d", *argv))
            ++debug;
        else
            usage ();
    }
    initdict ();
    fdin = dup (fileno (stdin));
    for (; argc>0; ++argv, --argc) {
        if (strcmp ("-", *argv) == 0) {
            fclose (stdin);
            fdopen (dup (fdin), "r");
        } else {
            FILE *fd = fopen (*argv, "r");
            if (! fd) {
                fprintf (stderr, "Cannot open %s\n", *argv);
                exit (1);
            }
            fclose (stdin);
            fdopen (dup (fileno (fd)), "r");
            fclose (fd);
        }
        forth ();
    }
    fclose (stdin);
    fdopen (dup (fdin), "r");
    close (fdin);
    forth ();
    return (0);
}

char *getword (int delim, int contflag)
{
    static char buf [256];
    char *p;
    int c;

    if (! contflag && boln)
        return (0);
    p = buf;
    for (;;) {
        if (boln) {
            if (tty)
                printf (errorflag ? ERRPROMPT : compilation ?
                    COMPILEPROMPT : OKPROMPT);
            boln = 0;
        }
        switch (c = getc (stdin)) {
        case -1:
            *p = 0;
            if (p <= buf) {
                if (inptr <= 0)
                    return (0);
                fclose (stdin);
                fdopen (dup (instack [--inptr]), "r");
                close (instack [inptr]);
                tty = isatty (fileno (stdin));
                boln = 1;
                continue;
            }
            return (buf);
        case '\n':
            boln = 1;
            goto seedelim;
        case 0:
            c = delim;
            goto seedelim;
        case '\t':
            c = ' ';
        default:
            if (c == delim) {
seedelim:                       *p = 0;
                if (contflag && p<=buf)
                    continue;
                return (buf);
            }
            if (p < buf+sizeof(buf)-1)
                *p++ = c;
            break;
        }
    }
}

int stackerr (int i)
{
    error (i ? "stack overflow" : "stack empty");
    return (0);
}

integer_t alloc (integer_t n)
{
    integer_t p;

    here = (here + sizeof (value_t) - 1) / sizeof (value_t) * sizeof (value_t);
    n *= sizeof (value_t);
    if (here+n > MEMSZ)
        error ("memory overflow");
    p = here;
    here += n;
    return (p);
}

integer_t allot (integer_t n)
{
    integer_t p;

    if (here+n > MEMSZ)
        error ("memory overflow");
    p = here;
    here += n;
    return (p);
}

void execute (value_t *a)
{
    if (execcnt >= 0) {
        if (execcnt >= NESTSZ)
            error ("too deep recursion");
        execstack [execcnt++] = execptr;
        execptr = a;
    } else {
        execptr = a;
        execcnt = 0;
    }
}

ident_t *find (char *name)
{
    ident_t *w;

    for (w=dictptr-1; w>=dict; --w)
        if (name[0]==w->name[0] && !strcmp (name, w->name))
            return (w);
    return (0);
}

ident_t *enter (char *name)
{
    ident_t *w;

    if (dictptr >= dict+DICTSZ)
        error ("dictionary overflow");
    w = dictptr;
    strncpy (w->name, name, NAMESZ);
    w->name [NAMESZ-1] = 0;
    w->type = 0;
    w->immed = 0;
    return (w);
}

char *funcname (funcptr_t p, int type)
{
    ident_t *w;

    for (w=dictptr-1; w>=dict; --w)
        if (type==w->type && p==w->val.p)
            return (w->name);
    return ("???");
}

void decompile (value_t *a)
{
    char *p;
    int c;

    for (; a->i; ++a) {
        switch (a->i) {
        case FSUBR:
            printf ("%s ", funcname ((++a)->p, FSUBR));
            continue;
        case FHARDWARE:
            printf ("%s ", funcname ((++a)->p, FHARDWARE));
            continue;
        case FQUOTE:
            printf ("\" ");
            goto decompstring;
        case FCSTRING:
            printf (".( ");
            c = (++a)->i;
            p = (char*) ++a;
            for (; *p; ++p)
                putc (*p, stdout);
            a += c / sizeof (value_t);
            printf (") ");
            continue;
        case FSTRING:
            printf (".\" ");
decompstring:           c = (++a)->i;
            p = (char*) ++a;
            for (; *p; ++p)
                putc (*p, stdout);
            a += c / sizeof (value_t);
            printf ("\" ");
            continue;
        case FGENINT:   printf ("%ld ", (++a)->i);  continue;
        case FGENFLT:   printf ("%g ", (double) (++a)->r); continue;
        case FCONDIF:   printf ("if ");             continue;
        case FCONDELSE: printf ("else ");           continue;
        case FUNTIL:    printf ("until ");          continue;
        case FREPEAT:   printf ("repeat ");         continue;
        case FWHILE:    printf ("while ");          continue;
        case FLEAVE:    printf ("leave ");          continue;
        case FDO:       printf ("do ");             continue;
        case FIFDO:     printf ("?do ");            continue;
        case FLOOP:     printf ("loop ");           continue;
        case FPLOOP:    printf ("+loop ");          continue;
        case FCOUNTI:   printf ("i ");              continue;
        case FCOUNTJ:   printf ("j ");              continue;
        case FCOUNTK:   printf ("k ");              continue;
        case FBEGIN:    printf ("begin ");          continue;
        case FCONDTHEN: printf ("then ");           continue;
        case FEXIT:     printf ("exit ");           continue;
        default:        printf ("?%ld ", a->i);     continue;
        }
    }
}
