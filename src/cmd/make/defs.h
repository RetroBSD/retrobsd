/* defs 4.10.1 1996/10/27 */
#ifdef CROSS
#   include </usr/include/stdio.h>
#   include </usr/include/ctype.h>
#else
#   include <stdio.h>
#   include <ctype.h>
#endif
#include <sys/param.h>
#include <sys/dir.h>

#define SHELLCOM "/bin/sh"

typedef long int TIMETYPE;

/*  to install metering, add a statement like
 * #define METERFILE "/usr/sif/make/Meter"
 * to turn metering on, set external variable meteron to 1.
 */

/* define FSTATIC to be static on systems with C compilers
   supporting file-static; otherwise define it to be null
*/
#define FSTATIC static

#define NO      0
#define YES     1

#define unequal strcmp

#define HASHSIZE        541     // 1021
#define NLEFTS          80      // 512
#define NCHARS          500
#define NINTS           250
#define INMAX           1350    // 7000
#define OUTMAX          2300    // 7000
#define MAXDIR          10
#define QBUFMAX         5000

#define ALLDEPS         1
#define SOMEDEPS        2

#define META            1
#define TERMINAL        2

extern char funny[128];

#define ALLOC(x) (struct x *) ckalloc(sizeof(struct x))

extern int sigivalue;
extern int sigqvalue;
extern int wpid;
extern int dbgflag;
extern int prtrflag;
extern int silflag;
extern int noexflag;
extern int keepgoing;
extern int noruleflag;
extern int touchflag;
extern int questflag;
extern int ndocoms;
extern int ignerr;
extern int okdel;
extern int inarglist;
extern char *prompt;
extern int nopdir;
extern char junkname[];
extern char *builtin[];

struct nameblock {
    struct nameblock *nxtnameblock;
    char *namep;
    char *alias;
    struct lineblock *linep;
    int done:3;
    int septype:3;
    TIMETYPE modtime;
};

extern struct nameblock *mainname;
extern struct nameblock *firstname;

struct lineblock {
    struct lineblock *nxtlineblock;
    struct depblock *depp;
    struct shblock *shp;
};
extern struct lineblock *sufflist;

struct depblock {
    struct depblock *nxtdepblock;
    struct nameblock *depname;
};

struct shblock {
    struct shblock *nxtshblock;
    char *shbp;
};

struct varblock {
    struct varblock *nxtvarblock;
    char *varname;
    char *varval;
    int noreset:1;
    int used:1;
};
extern struct varblock *firstvar;

struct pattern {
    struct pattern *nxtpattern;
    char *patval;
};
extern struct pattern *firstpat;

struct dirhdr {
    struct dirhdr *nxtopendir;
    DIR *dirfc;
    char *dirn;
};
extern struct dirhdr *firstod;

struct chain {
    struct chain *nextp;
    char *datap;
};

extern char **linesptr;

int yyparse (void);
void yyerror (char *s);
void enbint (void (*k)(int));
void intrupt (int sig);
int eqsign (char *a);
void setvar (char *v, char *s);
void fatal (char *s);
void fatal1 (char *s, char *t);
struct varblock *varptr (char *v);
struct nameblock *srchname (char *s);
struct nameblock *makename (char *s);
int doname (struct nameblock *p, int reclevel, TIMETYPE *tval);
char *subst (char *a, char *b);
TIMETYPE exists (struct nameblock *pname);
void expand (struct depblock *q);
int dosys (char *comstring, int nohalt);
void touch (int force, char *name);
void fixname (char *s, char *d);
int suffix (char *a, char *b, char *p);
char *concat (char *a, char *b, char *c);
char *copys (char *s);
struct depblock *srchdir (char *pat, int mkchain, struct depblock *nextdbl);
int *ckalloc (int n);
struct chain *appendq (struct chain *head, char *tail);
char *mkqlist (struct chain *p);
TIMETYPE prestime (void);
