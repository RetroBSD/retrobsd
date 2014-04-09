/*
 * Implementation of macro features.
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include "r.defs.h"

/*
 * Types of macros
 * TAG - poition in file
 * BUF - paste buffer
 */
#define MTAG 1
#define MBUF 2

typedef struct {
    int line, col, nfile;
} tag_t;

typedef union {
    clipboard_t mclipboard;
    tag_t mtag;
} macro_t;

#define NMACRO ('z'-'a'+1)

static macro_t *mtab_data[NMACRO];

static char mtab_type[NMACRO];

/*
 * Find a macro by name.
 * When nbytes=0, it finds and checks a type.
 * Otherwise, creates a new descriptor.
 */
static macro_t *mfind(name, typ, nbytes)
    register char *name;
    int typ, nbytes;
{
    register int i;

    i = ((*name | 040) & 0177) - 'a';
    if (i < 0 || i > 'z'-'a' || name[1] != 0) {
        error("Invalid macro name");
        return 0;
    }
    if (nbytes == 0) {
        /* Check the type of macro. */
        if (mtab_type[i] != typ) {
            error (mtab_type[i] ? "Invalid type of macro" : "Macro undefined");
            return 0;
        }
    } else {
        /* Reallocate. */
        if (mtab_data[i]) {
            free(mtab_data[i]);
            telluser("Macro redefined",0);
        }
        mtab_data[i] = (macro_t*) salloc(nbytes);
        mtab_type[i] = typ;
    }
    return mtab_data[i];
}

/*
 * Fetch a clipboard by name.
 * Return 0 on error.
 */
int mfetch(cb, name)
    register clipboard_t *cb;
    register char *name;
{
    register macro_t *m;

    m = mfind(name, MBUF, 0);
    if (! m)
        return 0;

    *cb = m->mclipboard;
    return 1;
}

/*
 * Store a clipboard by name.
 */
void mstore(cb, name)
    register clipboard_t *cb;
    register char *name;
{
    register macro_t *m;

    m = mfind(name, MBUF, sizeof(clipboard_t));
    if (m)
        m->mclipboard = *cb;
}

/*
 * Save a current cursor position in a file under the given name.
 * The deficiency is that the tag is not linked to a file
 * and moves when lines are inserted or deleted.
 */
int msvtag(name)
    register char *name;
{
    register macro_t *m;
    register workspace_t *cws;

    cws = curwksp;
    m = mfind(name, MTAG, sizeof(tag_t));
    if (! m)
        return 0;
    m->mtag.line  = cursorline + cws->topline;
    m->mtag.col   = cursorcol  + cws->offset;
    m->mtag.nfile = cws->wfile;
    return 1;
}

/*
 * Return a cursor back to named position.
 * cgoto is common for it and other functions.
 */
int mgotag(name)
    char *name;
{
    register int i;
    int fnew = 0;
    register macro_t *m;

    m = mfind(name, MTAG, 0);
    if (! m)
            return 0;
    i = m->mtag.nfile;
    if (curwksp->wfile != i) {
        editfile(file[i].name, 0, 0, 0, 0);
        fnew = 1;
    }
    cgoto(m->mtag.line, m->mtag.col, -1, fnew);
    highlight_position = 1;
    return 1;
}

/*
 * Define the parameters, describing the text area between the current
 * cursor and a given tag name.  Param_type is set to -2.
 */
int mdeftag(name)
    char *name;
{
    register macro_t *m;
    register workspace_t *cws;
    int cl, ln, f = 0;

    m = mfind(name, MTAG, 0);
    if (! m)
        return 0;
    cws = curwksp;
    if (m->mtag.nfile != cws->wfile) {
        error("File mismatch");
        return(0);
    }
    param_type = -2;
    param_r1 = m->mtag.line;
    param_c1 = m->mtag.col ;
    if (param_r0 > param_r1) {
        f++;
        ln = param_r1;
        param_r1 = param_r0;
        param_r0 = ln;
    } else
        ln = param_r0;

    if (param_c0 > param_c1) {
        f++;
        cl = param_c1;
        param_c1 = param_c0;
        param_c0 = cl;
    } else
        cl = param_c0;
    if (f) {
        cgoto(ln, cl, -1, 0);
    }
    if (param_r1 == param_r0)
        telluser("*** Columns defined by tag ***", 0);
    else if (param_c1 == param_c0)
        telluser("*** Lines defined by tag ***", 0);
    else
        telluser("*** Square defined by tag ***", 0);
    return 1;
}
