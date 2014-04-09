#include "defs.h"
#ifdef CROSS
#   include </usr/include/ctype.h>
#else
#   include <ctype.h>
#endif

static int savlastf;
static long savframe;
static int savpc;

static char isymbol[MAXSYMLEN + 2];

/*
 * service routines for expression reading
 */
static int
symchar(dig)
{
    if (lastc == '\\') {
        readchar();
        return TRUE;
    }
    return (isalpha(lastc) || lastc == '_' || (dig && isdigit(lastc)));
}

static void
readsym()
{
    register char *p;

    p = isymbol;
    do {
        if (p < &isymbol[MAXSYMLEN])
            *p++ = lastc;
        readchar();
    } while (symchar(1));
    *p++ = 0;
}

static void
chkloc(frame)
    long frame;
{
    readsym();
    do {
        if (localsym(frame) == 0)
            error(BADLOC);
        expv = localval;
    } while (! eqsym(cache_sym(symbol), isymbol, '~'));
}

static struct SYMbol *
lookupsym(symstr)
    char *symstr;
{
    register struct SYMbol *symp, *sc;

    symset();
    while ((symp = symget())) {
        sc = cache_by_string(symstr);
        if (sc)
            return sc;
        if (eqsym(no_cache_sym(symp), symstr, '_'))
            break;
    }

    /*
     * We did not enter anything into the cache (no sense inserting hundreds
     * of symbols which didn't match) while examining the (entire) symbol table.
     * Now that we have a match put it into the cache (doing a lookup on it is
     * the easiest way).
     */
    if (symp)
        (void)cache_sym(symp);
    return symp;
}

static int
convdig(c)
    int c;
{
    if (isdigit(c))
        return c - '0';
    if (isxdigit(c))
        return c - 'a' + 10;
    return 17;
}

/*
 * name [ . local ] | number | . | ^ | <var | <register | 'x |
 */
int
item(a)
{
    int base, d, frpt, regptr;
    char savc, hex;
    long frame;
    union {
        float r;
        long i;
    } real;
    register struct SYMbol *symp;

    hex = FALSE;

    readchar();
    if (symchar(0)) {
        readsym();
        if (lastc == '.') {
            frame = (kernel ? corhdr[KR5] : uframe[FRAME_R5]) & ALIGNED;
            lastframe = 0;
            callpc = kernel ? (-2) : uframe[FRAME_PC];
            while (errflg == 0) {
                savpc = callpc;
                findroutine(frame);
                if (eqsym(cache_sym(symbol), isymbol, '~'))
                    break;
                lastframe = frame;
                frame = get(frame, DSP) & ALIGNED;
                if (frame == 0) {
                    error(NOCFN);
                }
            }
            savlastf = lastframe;
            savframe = frame;
            readchar();
            if (symchar(0)) {
                expv = frame;
                chkloc(expv);
            }
        } else if ((symp = lookupsym(isymbol)) == 0) {
            error(BADSYM);
        } else {
            expv = symp->value;
        }
        lp--;

    } else if (isdigit(lastc) ||
        (hex = TRUE, lastc == '#' && isxdigit(readchar())))
    {
        expv = 0;
        base = (lastc == '0' || octal) ? 8 : (hex ? 16 : 10);
        while ((hex ? isxdigit(lastc) : isdigit(lastc))) {
            expv *= base;
            d = convdig(lastc);
            if (d >= base) {
                error(BADSYN);
            }
            expv += d;
            readchar();
            if (expv == 0 && (lastc == 'x' || lastc == 'X')) {
                hex = TRUE;
                base = 16;
                readchar();
            }
        }
        if (lastc == '.' && (base == 10 || expv == 0) && ! hex) {
            real.r = expv;
            frpt = 0;
            base = 10;
            while (isdigit(readchar())) {
                real.r *= base;
                frpt++;
                real.r += lastc - '0';
            }
            while (frpt--) {
                real.r /= base;
            }
            expv = real.i;
        }
        lp--;

    } else if (lastc == '.') {
        readchar();
        if (symchar(0)) {
            lastframe = savlastf;
            callpc = savpc;
            findroutine(savframe);
            chkloc(savframe);
        } else {
            expv = dot;
        }
        lp--;

    } else if (lastc == '"') {
        expv = ditto;

    } else if (lastc == '+') {
        expv = inkdot(dotinc);

    } else if (lastc == '^') {
        expv = inkdot(-dotinc);

    } else if (lastc == '<') {
        savc = rdc();
        regptr = getreg(savc);
        if (regptr != NOREG) {
            expv = uframe[regptr];
        } else if ((base = varchk(savc)) != -1) {
            expv = var[base];
        } else {
            error(BADVAR);
        }
    } else if (lastc == '\'') {
        d = 4;
        expv = 0;
        while (quotchar()) {
            if (d--) {
                if (d == 1)
                    expv <<= 16;
                expv |= (d & 1) ? lastc : (lastc << 8);
            } else {
                error(BADSYN);
            }
        }
    } else if (a) {
        error(NOADR);
    } else {
        lp--;
        return 0;
    }
    return 1;
}

/*
 * item | monadic item | (expr) |
 */
int
term(a)
{
    switch (readchar()) {

    case '*':
        term(a | 1);
        expv = chkget(expv, DSP);
        return 1;

    case '@':
        term(a | 1);
        expv = chkget(expv, ISP);
        return 1;

    case '-':
        term(a | 1);
        expv = -expv;
        return 1;

    case '~':
        term(a | 1);
        expv = ~expv;
        return 1;

    case '(':
        expr(2);
        if (*lp != ')') {
            error(BADSYN);
        } else {
            lp++; return 1;
        }

    default:
        lp--;
        return item(a);
    }
}

/*
 * term | term dyadic expr |
 */
int
expr(a)
{
    int rc;
    long lhs;

    rdc();
    lp--;
    rc = term(a);

    while (rc) {
        lhs = expv;
        switch (readchar()) {
        case '+': term(a|1); expv += lhs; break;
        case '-': term(a|1); expv = lhs - expv; break;
        case '#': term(a|1); expv = roundn(lhs, expv); break;
        case '*': term(a|1); expv *= lhs; break;
        case '%': term(a|1); expv = lhs/expv; break;
        case '&': term(a|1); expv &= lhs; break;
        case '|': term(a|1); expv |= lhs; break;
        case ')': if (! (a & 2)) error(BADKET);
        default:  lp--; return rc;
        }
    }
    return rc;
}

int
varchk(name)
{
    if (isdigit(name))
        return name - '0';
    if (isalpha(name))
        return (name & 037) - 1 + 10;
    return -1;
}

int
eqsym(s1, s2, c)
    register char *s1, *s2;
    int c;
{
    if (! strcmp(s1, s2))
        return TRUE;
    if (*s1++ == c)
        return ! strcmp(s1, s2);
    return FALSE;
}
