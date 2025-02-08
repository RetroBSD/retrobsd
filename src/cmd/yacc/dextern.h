/*
 * @(#)dextern	4.2	(Berkeley)	3/21/86
 */
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#include "files.h"

/*  MANIFEST CONSTANT DEFINITIONS */

/* base of nonterminal internal numbers */
#define NTBASE 010000

/* internal codes for error and accept actions */

#define ERRCODE 8190
#define ACCEPTCODE 8191

/* sizes and limits */

#ifdef HUGE
#define ACTSIZE 12000
#define MEMSIZE 24000
#define NSTATES 750
#define NTERMS 300
#define NPROD 600
#define NNONTERM 300
#define TEMPSIZE 1200
#define CNAMSZ 5000
#define LSETSIZE 600
#define WSETSIZE 350
#endif

#ifdef MEDIUM
#define ACTSIZE 4000
#define MEMSIZE 5200
#define NSTATES 750
#define NTERMS 127
#define NPROD 400
#define NNONTERM 200
#define TEMPSIZE 800
#define CNAMSZ 4000
#define LSETSIZE 450
#define WSETSIZE 250
#endif

#ifdef TINY
#define ACTSIZE 1000
#define MEMSIZE 1300
#define NSTATES 750
#define NTERMS 127
#define NPROD 200
#define NNONTERM 50
#define TEMPSIZE 200
#define CNAMSZ 1000
#define LSETSIZE 250
#define WSETSIZE 100
#endif

#define NAMESIZE 50
#define NTYPES 63

#ifdef WORD32
#define TBITSET ((32 + NTERMS) / 32)

/* bit packing macros (may be machine dependent) */
#define BIT(a, i) ((a)[(i) >> 5] & (1 << ((i)&037)))
#define SETBIT(a, i) ((a)[(i) >> 5] |= (1 << ((i)&037)))

/* number of words needed to hold n+1 bits */
#define NWORDS(n) (((n) + 32) / 32)

#else

#define TBITSET ((16 + NTERMS) / 16)

/* bit packing macros (may be machine dependent) */
#define BIT(a, i) ((a)[(i) >> 4] & (1 << ((i)&017)))
#define SETBIT(a, i) ((a)[(i) >> 4] |= (1 << ((i)&017)))

/* number of words needed to hold n+1 bits */
#define NWORDS(n) (((n) + 16) / 16)
#endif

/* relationships which must hold:
TBITSET ints must hold NTERMS+1 bits...
WSETSIZE >= NNONTERM
LSETSIZE >= NNONTERM
TEMPSIZE >= NTERMS + NNONTERMs + 1
TEMPSIZE >= NSTATES
*/

/* associativities */

#define NOASC 0 /* no assoc. */
#define LASC 1  /* left assoc. */
#define RASC 2  /* right assoc. */
#define BASC 3  /* binary assoc. */

/* flags for state generation */

#define DONE 0
#define MUSTDO 1
#define MUSTLOOKAHEAD 2

/* flags for a rule having an action, and being reduced */

#define ACTFLAG 04
#define REDFLAG 010

/* output parser flags */
#define YYFLAG1 (-1000)

/* macros for getting associativity and precedence levels */

#define ASSOC(i) ((i)&03)
#define PLEVEL(i) (((i) >> 4) & 077)
#define TYPE(i) ((i >> 10) & 077)

/* macros for setting associativity and precedence levels */

#define SETASC(i, j) i |= j
#define SETPLEV(i, j) i |= (j << 4)
#define SETTYPE(i, j) i |= (j << 10)

/* looping macros */

#define TLOOP(i) for (i = 1; i <= ntokens; ++i)
#define NTLOOP(i) for (i = 0; i <= nnonter; ++i)
#define PLOOP(s, i) for (i = s; i < nprod; ++i)
#define SLOOP(i) for (i = 0; i < nstate; ++i)
#define WSBUMP(x) ++x
#define WSLOOP(s, j) for (j = s; j < cwp; ++j)
#define ITMLOOP(i, p, q) \
    q = pstate[i + 1];   \
    for (p = pstate[i]; p < q; ++p)
#define SETLOOP(i) for (i = 0; i < tbitset; ++i)

/* I/O descriptors */

extern FILE *finput;  /* input file */
extern FILE *faction; /* file for saving actions */
extern FILE *fdefine; /* file for # defines */
extern FILE *ftable;  /* y.tab.c file */
extern FILE *ftemp;   /* tempfile to pass 2 */
extern FILE *foutput; /* y.output file */

/* structure declarations */

struct looksets {
    int lset[TBITSET];
};

struct item {
    int *pitem;
    struct looksets *look;
};

struct toksymb {
    char *name;
    int value;
};

struct ntsymb {
    char *name;
    int tvalue;
};

struct wset {
    int *pitem;
    int flag;
    struct looksets ws;
};

/* token information */

extern int ntokens; /* number of tokens */
extern struct toksymb tokset[];
extern int toklev[]; /* vector with the precedence of the terminals */

/* nonterminal information */

extern int nnonter; /* the number of nonterminals */
extern struct ntsymb nontrst[];

/* grammar rule information */

extern int nprod;     /* number of productions */
extern int *prdptr[]; /* pointers to descriptions of productions */
extern int levprd[];  /* contains production levels to break conflicts */

/* state information */

extern int nstate;            /* number of states */
extern struct item *pstate[]; /* pointers to the descriptions of the states */
extern int tystate[];         /* contains type information about the states */
extern int defact[];          /* the default action of the state */
extern int tstates[];         /* the states deriving each token */
extern int ntstates[];        /* the states deriving each nonterminal */
extern int mstates[];         /* the continuation of the chains begun in tstates and ntstates */

/* lookahead set information */

extern struct looksets lkst[];
extern int nolook; /* flag to turn off lookahead computations */

/* working set information */

extern struct wset wsets[];
extern struct wset *cwp;

/* storage for productions */

extern int mem0[];
extern int *mem;

/* storage for action table */

extern int amem[];  /* action table storage */
extern int *memp;   /* next free action table position */
extern int indgo[]; /* index to the stored goto table */

/* temporary vector, indexable by states, terms, or ntokens */

extern int temp1[];
extern int lineno; /* current line number */

/* statistics collection variables */

extern int zzgoent;
extern int zzgobest;
extern int zzacent;
extern int zzexcp;
extern int zzclose;
extern int zzrrconf;
extern int zzsrconf;
/* define functions with strange types... */

extern char *cstash();
extern struct looksets *flset();
extern char *symnam();
extern char *writem();

/* default settings for a number of macros */

/* name of yacc tempfiles */

#ifndef TEMPNAME
#define TEMPNAME "yacc.tmp"
#endif

#ifndef ACTNAME
#define ACTNAME "yacc.acts"
#endif

/* output file name */

#ifndef OFILE
#define OFILE "y.tab.c"
#endif

/* user output file name */

#ifndef FILEU
#define FILEU "y.output"
#endif

/* output file for # defines */

#ifndef FILED
#define FILED "y.tab.h"
#endif

/* command to clobber tempfiles after use */

#ifndef ZAPFILE
#define ZAPFILE(x) unlink(x)
#endif

void setup(int argc, char *argv[]);
void output(void);
void go2out(void);
void hideprod(void);
void callopt(void);
void error(char *fmt, ...);
void warray(char *s, int *v, int n);
void aryfil(int *v, int n, int c);
void closure(int i);
int apack(int *p, int n);
void putitem(int *ptr, struct looksets *lptr);
int state(int c);
