#ifdef CROSS
#   include </usr/include/stdio.h>
#else
#   include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#   define assert(x) { if (! (x)) {\
	fprintf (stderr, "assertion failed: file \"%s\", line %d\n",\
	__FILE__, __LINE__); exit (-1); } }
#else
#   define assert(x)
#endif
#define INLINE static inline __attribute__((always_inline))

/*
 * Type tags.
 */
#define TPAIR           1               /* tag "pair" */
#define TSYMBOL         2               /* tag "symbol" */
#define TBOOL           3               /* tag "boolean" */
#define TCHAR           4               /* tag "character" */
#define TINTEGER        5               /* tag "integer" */
#define TREAL           6               /* tag "real" */
#define TSTRING         7               /* tag "string" */
#define TVECTOR         8               /* tag "vector" */
#define THARDW          9               /* tag "hard-wired function" */
#define TCLOSURE       10               /* tag "closure" */

#define NIL             0xffffff        /* empty list */
#define TOPLEVEL        0xfffffe        /* top level context */

typedef unsigned lisp_t;                /* address of a cell */
typedef lisp_t (*func_t) (lisp_t, lisp_t);  /* pointer to hardwired function */

typedef union {                         /* elementary cell, two words */
        unsigned type : 8;              /* type */
        struct {                        /* pair */
                unsigned type : 8;      /* type */
                unsigned a : 24;        /* first element */
                size_t d;               /* second element - should be... */
        } pair;                         /* ...large enough to keep a pointer */
        struct {                        /* string */
                unsigned type : 8;      /* type */
                unsigned length : 24;   /* length */
                char *array;            /* array of bytes */
        } string;
        struct {                        /* vector */
                unsigned type : 8;      /* type */
                unsigned length : 24;   /* length */
                lisp_t *array;          /* array of elements */
        } vector;
        struct {                        /* vector */
                unsigned type : 8;      /* type */
		char *name;             /* c string pointer */
        } symbol;
        struct {                        /* vector */
                unsigned type : 8;      /* type */
		unsigned value;         /* character */
        } chr;
        struct {                        /* vector */
                unsigned type : 8;      /* type */
		long value;             /* integer number, hardwired function */
        } integer;
        struct {                        /* vector */
                unsigned type : 8;      /* type */
		double value;           /* real number */
        } real;
} cell_t;

typedef struct {
	char *name;
	func_t func;
} functab_t;

extern lisp_t T, ZERO, ENV;

int eqv (lisp_t a, lisp_t b);                   /* compare objects */
int equal (lisp_t a, lisp_t b);                 /* recursive comparison */
lisp_t evalblock (lisp_t expr, lisp_t ctx);     /* evaluation of a block */
lisp_t evalclosure (lisp_t func, lisp_t expr);  /* evaluation of a closure */
lisp_t evalfunc (lisp_t func, lisp_t arg, lisp_t ctx); /* evaluation of a function */
lisp_t eval (lisp_t expr, lisp_t *ctxp);        /* evaluation */
lisp_t getexpr ();                              /* read expression */
void putexpr (lisp_t p, FILE *fd);              /* print expression */
lisp_t copy (lisp_t a, lisp_t *t);              /* copy expression */
lisp_t alloc (int type);                        /* allocate a cell */
void fatal (char*);                             /* fatal error */
int istype (lisp_t p, int type);                /* check object type */

extern cell_t mem[];                            /* main storage for cells */
extern unsigned memsz;                          /* size of storage */
extern void *memcopy (void*, int);

INLINE lisp_t car (lisp_t p)            /* get first element of a pair */
{
	assert (p>=0 && p<memsz && mem[p].type==TPAIR);
	return mem[p].pair.a;
}

INLINE lisp_t cdr (lisp_t p)            /* get second element of a pair */
{
	assert (p>=0 && p<memsz && mem[p].type==TPAIR);
	return mem[p].pair.d;
}

INLINE void setcar (lisp_t p, lisp_t v) /* set first element */
{
	assert (p>=0 && p<memsz && mem[p].type==TPAIR);
	mem[p].pair.a = v;
}

INLINE void setcdr (lisp_t p, lisp_t v) /* set second element */
{
	assert (p>=0 && p<memsz && mem[p].type==TPAIR);
	mem[p].pair.d = v;
}

INLINE lisp_t cons (lisp_t a, lisp_t d) /* allocate a pair */
{
	lisp_t p = alloc (TPAIR);
	setcar (p, a);
	setcdr (p, d);
	return p;
}

INLINE lisp_t symbol (char *name)       /* allocate a symbol (atom) */
{
	lisp_t p = alloc (TSYMBOL);
	mem[p].symbol.name = memcopy (name, strlen (name) + 1);
	return p;
}

INLINE long numval (lisp_t p)           /* get value of an integer */
{
	assert (p>=0 && p<memsz && mem[p].type==TINTEGER);
	return mem[p].integer.value;
}

INLINE lisp_t number (long val)         /* allocate integer number */
{
	lisp_t p = alloc (TINTEGER);
	mem[p].integer.value = val;
	return p;
}

INLINE lisp_t string (int len, char *array) /* allocate a string */
{
	lisp_t p = alloc (TSTRING);
	mem[p].string.length = len;
	if (len > 0)
		mem[p].string.array = memcopy (array, len);
	return p;
}

INLINE long charval (lisp_t p)          /* get value of character */
{
	assert (p>=0 && p<memsz && mem[p].type==TCHAR);
	return mem[p].chr.value;
}

INLINE lisp_t character (int val)       /* allocate a character */
{
	lisp_t p = alloc (TCHAR);
	mem[p].chr.value = val;
	return p;
}

INLINE lisp_t real (double val)         /* allocate a real number */
{
	lisp_t p = alloc (TREAL);
	mem[p].real.value = val;
	return p;
}

INLINE lisp_t vector (int len, lisp_t *array) /* allocate a vector */
{
	lisp_t p = alloc (TVECTOR);
	mem[p].vector.length = len;
	if (len > 0)
		mem[p].vector.array = memcopy (array, len * sizeof (lisp_t));
	return p;
}

INLINE lisp_t closure (lisp_t body, lisp_t ctx) /* allocate a closure */
{
	lisp_t p = alloc (TCLOSURE);
	mem[p].pair.a = body;
	mem[p].pair.d = ctx;
	return p;
}

INLINE lisp_t hardw (func_t func)       /* allocate a hard-wired function */
{
	lisp_t p = alloc (THARDW);
	mem[p].integer.value = (long) func;
	return p;
}

INLINE double realval (lisp_t p)        /* get value of a real number */
{
	assert (p>=0 && p<memsz && mem[p].type==TREAL);
	return mem[p].real.value;
}

INLINE func_t hardwval (lisp_t p)       /* get address of hard-wired function */
{
	assert (p>=0 && p<memsz && mem[p].type==THARDW);
	return (func_t) mem[p].integer.value;
}

INLINE lisp_t closurebody (lisp_t p)    /* get a body of closure */
{
	assert (p>=0 && p<memsz && mem[p].type==TCLOSURE);
	return mem[p].pair.a;
}

INLINE lisp_t closurectx (lisp_t p)     /* get a context of closure */
{
	assert (p>=0 && p<memsz && mem[p].type==TCLOSURE);
	return mem[p].pair.d;
}

INLINE char *symname (lisp_t p)         /* get a name of a symbol */
{
	assert (p>=0 && p<memsz && mem[p].type==TSYMBOL);
	return mem[p].symbol.name;
}
