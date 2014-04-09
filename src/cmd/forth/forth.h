/*
 * Portable Forth interpreter.
 * Copyright (C) 1990-2012 Serge Vakulenko, <vak@cronyx.ru>
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
#include <setjmp.h>

#define MEMSZ           4096    /* max size of allot'ated memory */
#define DEFSZ           512     /* max size of compiled function */
#define STACKSZ         256     /* max size of data stack */
#define NAMESZ          32      /* max length of ident name */
#define DICTSZ          256     /* max size of dictionary */
#define NESTSZ          512     /* max depth of function calls nesting */
#define LOOPSZ          32      /* max loop nesting */
#define OUTSZ           8       /* max output file nesting */
#define INSZ            8       /* max input file nesting */

/* types of idents */

#define FGENINT         1       /* constant */
#define FGENFLT         2       /* float constant */
#define FSUBR           3       /* compiled function */
#define FHARDWARE       4       /* builtin function */
#define FSTRING         5       /* ." */
#define FCSTRING        6       /* .( */
#define FQUOTE          7       /* " */
#define FCQUOTE         8       /* ascii */
                                /* */
#define FREPEAT         10      /* repeat */
#define FDO             11      /* do */
#define FLOOP           12      /* loop */
#define FPLOOP          13      /* +loop */
#define FCONDIF         14      /* if */
#define FCONDELSE       15      /* else */
#define FCONDTHEN       16      /* then */
#define FBEGIN          17      /* begin */
#define FUNTIL          18      /* until */
#define FWHILE          19      /* while */
#define FCOUNTI         20      /* i */
#define FCOUNTJ         21      /* j */
#define FCOUNTK         22      /* k */
#define FLEAVE          23      /* leave */
#define FIFDO           24      /* ?do */
#define FEXIT           25      /* exit */

#define FETCHWORD(a)    (memory.v [(a) / sizeof(value_t)])
#define STOREWORD(a,w)  (memory.v [(a) / sizeof(value_t)] = (w))
#define FETCHBYTE(a)    ((int) (unsigned char) memory.c [a])
#define STOREBYTE(a,w)  (memory.c [a] = (w))

#define compilation     FETCHWORD (compindex).i
#define base            FETCHWORD (baseindex).i
#define span            FETCHWORD (spanindex).i
#define hld             FETCHWORD (hldindex).i
#define pad             (MEMSZ-1)

#define push(i)         ((stackptr < stack+STACKSZ) ? \
                        *stackptr++ = (i) : stackerr (1))
#define fpush(f)        ((stackptr < stack+STACKSZ) ? \
                        *stackptr++ = ftoi (f) : stackerr (1))

#define pop()           ((stackptr > stack) ? *--stackptr : \
                        (integer_t) stackerr (0))
#define fpop()          ((stackptr > stack) ? itof (*--stackptr) : \
                        (real_t) stackerr (0))

/*
 * Size of integer_t must be equal to size of real_t.
 * All known machines have longs and floats of the same size,
 * so we will use these types.
 */
typedef long            integer_t;
typedef unsigned long   uinteger_t;
typedef float           real_t;
typedef void            (*funcptr_t) (void);

typedef union value {
    integer_t   i;
    real_t      r;
    union value *v;
    funcptr_t   p;
} value_t;

typedef struct ident {
    int         type;
    int         immed;
    value_t     val;
    char        name [NAMESZ];
} ident_t;

struct table {
    char        *name;
    funcptr_t   func;
    int         type;
    int         immed;
};

struct loop {
    int         type;
    integer_t   cnt;
    integer_t   low;
    integer_t   up;
    value_t     *ptr;
};

union memory {
    char        c [MEMSZ];
    value_t     v [MEMSZ / sizeof(value_t)];
};

enum {
    BINARY  = 2,
    OCTAL   = 8,
    DECIMAL = 10,
    HEX     = 16,
};

static inline integer_t ftoi (real_t z)
{
    value_t x;

    x.r = z;
    return x.i;
}

static inline real_t itof (integer_t z)
{
    value_t x;

    x.i = z;
    return x.r;
}

static inline value_t itov (integer_t z)
{
    value_t x;

    x.i = z;
    return x;
}

extern  ident_t         dict [];
extern  ident_t         *dictptr;
extern  ident_t         *lowdictptr;
extern  jmp_buf         errjmp;
extern  value_t         defbuf [];
extern  value_t         *defptr;
extern  union memory    memory;
extern  integer_t       here;
extern  integer_t       lowmem;
extern  integer_t       stack [];
extern  integer_t       *stackptr;
extern  struct table    func_table [];
extern  integer_t       compindex;
extern  integer_t       baseindex;
extern  integer_t       spanindex;
extern  integer_t       hldindex;
extern  int             outstack [];
extern  int             outptr;
extern  int             instack [];
extern  int             inptr;
extern  int             tty;
extern  int             boln;

extern  value_t         *execptr;
extern  int             execcnt;

extern  void            error (char *fmt, ...);
extern  ident_t         *find (char *name);
extern  ident_t         *enter (char *name);
extern  char            *getword (int delim, int contflag);
extern  integer_t       allot (integer_t n);
extern  integer_t       alloc (integer_t n);
extern  void            sexecute (ident_t *w);
extern  void            execute (value_t *a);
extern  int             stackerr (int i);
extern  void            forthresume (int clrflag);
extern  void            decompile (value_t *a);
