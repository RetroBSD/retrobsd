/*
 * Symbol table and file handling service routines
 */
#include "defs.h"
#include <fcntl.h>
#ifdef CROSS
#   include </usr/include/stdio.h>
#else
#   include <stdio.h>
#endif

struct  SYMcache {
    char    *name;
    int     used;
    struct  SYMbol *syment;
};

#ifndef NUM_SYMS_CACHE
#define NUM_SYMS_CACHE  50
#endif

static  struct  SYMcache symcache[NUM_SYMS_CACHE];
static  struct  SYMcache *endcache = &symcache[NUM_SYMS_CACHE];
static  struct  SYMbol  *symtab;
static  FILE    *strfp;
static  int     symcnt;
static  int     symnum;

u_int
findsym(svalue, type)
    u_int   svalue;
    int     type;
{
    long    diff, value, symval;
    register struct SYMbol  *sp;
    struct  SYMbol *symsav = 0;
    int     i;

    value = svalue;
    diff = 0xffffff;

    if (type != NSYM && symnum) {
        for (i = 0, sp = symtab; diff && i < symnum; i++, sp++) {
            if (SYMTYPE(sp->type) == type) {
                symval = leng(sp->value);
                if ((value - symval) < diff && value >= symval) {
                    diff = value - symval;
                    symsav = sp;
                }
            }
        }
    }
    if (symsav)
        symcnt = symsav - symtab;
    symbol = symsav;
    return shorten(diff);
}

void
valpr(v, idsp)
{
    u_int   d;

    d = findsym(v, idsp);
    if (d < maxoff) {
        print("%s", cache_sym(symbol));
        if (d)
            print(OFFMODE, d);
    }
}

int
localsym(cframe)
    long cframe;
{
    int symflg;

    while (symget() && localok && (symflg = (int)symbol->type) != N_FN &&
        *no_cache_sym(symbol) != '~')
    {
        if (symflg >= 2 && symflg <= 4) {
            localval = symbol->value;
            return TRUE;
        } else if (symflg == 1) {
            localval = leng(shorten(cframe) + symbol->value);
            return TRUE;
        } else if (symflg == 20 && lastframe) {
            localval = leng(lastframe + 2 * symbol->value - 10);
            return TRUE;
        }
    }
    return FALSE;
}

void
psymoff(v, type, s)
    long v;
    int type;
    char *s;
{
    u_int w;

    w = findsym(shorten(v), type);
    if (w >= maxoff) {
        print(LPRMODE, v);
    } else {
        print("%s", cache_sym(symbol));
        if (w) {
            print(OFFMODE, w);
        }
    }
    print(s);
}

/*
 *  sequential search through table
 */
void
symset()
{
    symcnt = -1;
}

struct SYMbol *
symget()
{
    if (symcnt >= symnum || ! symnum)
        return NULL;
    symcnt++;
    symbol = &symtab[symcnt];
    return symbol;
}

static unsigned int
fgetword (f)
    register FILE *f;
{
        register unsigned int h;

        h = getc (f);
        h |= getc (f) << 8;
        h |= getc (f) << 16;
        h |= getc (f) << 24;
        return h;
}

/*
 * Read a symbol table entry.
 * Return a number of bytes read, or -1 on EOF.
 * Format of symbol record:
 *  1 byte: length of name in bytes
 *  1 byte: type of symbol (N_UNDF, N_ABS, N_TEXT, etc)
 *  4 bytes: value
 *  N bytes: name
 */
static int
fgetsym (fi, name, value, type)
        register FILE *fi;
        register char *name;
        unsigned *value, *type;
{
        register int len;
        unsigned nbytes;

        len = getc (fi);
        if (len <= 0)
                return -1;
        *type = getc (fi);
        *value = fgetword (fi);
        nbytes = len + 6;
        if (name) {
                while (len-- > 0)
                        *name++ = getc (fi);
                *name = '\0';
        } else
                fseek (fi, len, SEEK_CUR);
        return nbytes;
}

/*
 * This only _looks_ expensive ;-)  The extra scan over the symbol
 * table allows us to cut down the amount of memory needed.
 * A late addition to the program excludes register symbols - the assembler
 * generates *lots* of them and they're useless to us.
 */
void
symINI(ex)
    struct exec *ex;
{
    register struct SYMbol  *sp;
    register FILE   *fp;
    int     nused, globals_only = 0;
    u_int   value, type;

    fp = fopen(symfil, "r");
    strfp = fp;
    if (! fp)
        return;
    fcntl(fileno(fp), F_SETFD, 1);

    /* First pass: count the number of symbols used. */
    fseek(fp, symoff, SEEK_SET);
    nused = 0;
    symnum = ex->a_syms;
    while (symnum > 0) {
        int nbytes = fgetsym(fp, 0, &value, &type);
        if (nbytes <= 0)
                break;
        symnum -= nbytes;
        nused++;
    }

    symtab = (struct SYMbol *)malloc(nused * sizeof (struct SYMbol));
    if (! symtab) {
        /* Second pass: count only globals. */
        fseek(fp, symoff, SEEK_SET);
        globals_only = 1;
        nused = 0;
        symnum = ex->a_syms;
        while (symnum > 0) {
            int nbytes = fgetsym(fp, 0, &value, &type);
            if (nbytes <= 0)
                    break;
            symnum -= nbytes;
            if (! (type & N_EXT))
                continue;
            nused++;
        }
        symtab = (struct SYMbol *)malloc(nused * sizeof(struct SYMbol));
        if (! symtab) {
            print("%s: no memory for %d symbols\n", myname, nused);
            symnum = 0;
            return;
        }
    }

    /* Third pass: read the symbols. */
    fseek(fp, symoff, SEEK_SET);
    sp = symtab;
    symnum = ex->a_syms;
    while (symnum > 0) {
        char name[256];
        int nbytes = fgetsym(fp, name, &value, &type);
        if (nbytes <= 0)
                break;
        symnum -= nbytes;
        if (globals_only && ! (type & N_EXT))
            continue;
        sp->value = value;
        sp->type = type;
        sp->sname = strdup(name);
        sp++;
//print("%s = %08x (%x)\n", name, value, type);
    }
    symnum = nused;
#if 1
    //printf("file '%s'\n", symfil);
    printf("text=%u data=%u bss=%u symoff=%u syms=%u\n",
        ex->a_text, ex->a_data, ex->a_bss, (unsigned) symoff, ex->a_syms);
    printf("%d symbols loaded\n", nused);
#endif
    if (globals_only)
        print("%s: could only do global symbols\n", myname);
    symset();
}

/*
 * Look in the cache for a symbol in memory.  If it is not found use
 * the least recently used entry in the cache and update it with the
 * symbol name.
*/
char *
cache_sym(symp)
    register struct SYMbol *symp;
{
    register struct SYMcache *sc = symcache;
    struct  SYMcache *current;
    int     lru;

    if (! symp)
        return "?";
    for (current = NULL, lru = 30000 ; sc < endcache; sc++) {
        if (sc->syment == symp) {
            sc->used++;
            if (sc->used >= 30000)
                sc->used = 10000;
            return sc->name;
        }
        if (sc->used < lru) {
            lru = sc->used;
            current = sc;
        }
    }
    sc = current;
    sc->used = 1;
    sc->syment = symp;
    sc->name = symp->sname;
    return sc->name;
}

/*
 * We take a look in the cache but do not update the cache on a miss.
 * This is done when scanning thru the symbol table (printing all externals
 * for example) for large numbers of symbols which probably won't be
 * used again any time soon.
 */
char *
no_cache_sym(symp)
    register struct SYMbol *symp;
{
    register struct SYMcache *sc = symcache;

    if (! symp)
        return "?";
    for ( ; sc < endcache; sc++) {
        if (sc->syment == symp) {
            sc->used++;
            if (sc->used >= 30000)
                sc->used = 10000;
            return sc->name;
        }
    }
    return symp->sname;
}

/*
 * Looks in the cache for a match by string value rather than string
 * file offset.
 */
struct SYMbol *
cache_by_string(str)
    char *str;
{
    register struct SYMcache *sc;

    for (sc = symcache; sc < endcache; sc++) {
        if (! sc->name)
            continue;
        if (eqsym(sc->name, str, '_'))
            break;
    }
    if (sc < endcache) {
        sc->used++;
        if (sc->used > 30000)
            sc->used = 10000;
        return sc->syment;
    }
    return 0;
}
