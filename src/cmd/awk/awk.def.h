/*	awk.def	4.3	83/12/09	*/

#define hack            int
#define	AWKFLOAT	float
#define	xfree(a)	{ if(a!=NULL) { yfree(a); a=NULL;} }
#define	strfree(a)	{ if(a!=NULL && a!=EMPTY) { yfree(a);} a=EMPTY; }
#define yfree           free
#define	isnull(x)	((x) == EMPTY || (x) == NULL)

#ifdef DEBUG
#   define dprintf	if(dbg)printf
#else
#   define dprintf(x1, x2, x3, x4)
#endif
typedef	AWKFLOAT	awkfloat;

extern char	**FS;
extern char	**RS;
extern char	**ORS;
extern char	**OFS;
extern char	**OFMT;
extern awkfloat *NR;
extern awkfloat *NF;
extern char	**FILENAME;

extern char	record[];
extern char	EMPTY[];
extern int	dbg;
extern int	lineno;
extern int	errorflag;
extern int	donefld;	/* 1 if record broken into fields */
extern int	donerec;	/* 1 if record is valid (no fld has changed */

typedef struct val {	/* general value during processing */
	char	*nval;	/* name, for variables only */
	char	*sval;	/* string value */
	awkfloat	fval;	/* value as number */
	unsigned	tval;	/* type info */
	struct val	*nextval;	/* ptr to next if chained */
} cell;
extern cell *symtab[];
cell	*setsymtab(), *lookup(), **makesymtab();

extern cell	*recloc;	/* location of input record */
extern cell	*nrloc;		/* NR */
extern cell	*nfloc;		/* NF */

#define	STR	01	/* string value is valid */
#define	NUM	02	/* number value is valid */
#define FLD	04	/* FLD means don't free string space */
#define	CON	010	/* this is a constant */
#define	ARR	020	/* this is an array */

awkfloat setfval(), getfval();
char	*setsval(), *getsval();
char	*tostring(), *tokname();

/* function types */
#define	FLENGTH	1
#define	FSQRT	2
#define	FEXP	3
#define	FLOG	4
#define	FINT	5

typedef struct {
	char otype;
	char osub;
	cell *optr;
} obj;

#define BOTCH	0
struct nd {
	char ntype;
	char subtype;
	struct nd *nnext;
	int nobj;
	struct nd *narg[BOTCH];	/* C won't take a zero length array */
};
typedef struct nd node;
extern node	*winner;
extern node	*nullstat;

/* otypes */
#define OCELL	0
#define OEXPR	1
#define OBOOL	2
#define OJUMP	3

/* cell subtypes */
#define CTEMP	4
#define CNAME	3
#define CVAR	2
#define CFLD	1
#define CCON	0

/* bool subtypes */
#define BTRUE	1
#define BFALSE	2

/* jump subtypes */
#define JEXIT	1
#define JNEXT	2
#define	JBREAK	3
#define	JCONT	4

/* node types */
#define NVALUE	1
#define NSTAT	2
#define NEXPR	3
#define NPA2	4

typedef obj	(*objfunc)();
extern const objfunc proctab[];
extern obj	true, false;
extern int	pairstack[], paircnt;

#define cantexec(n)	(n->ntype == NVALUE)
#define notlegal(n)	(n <= FIRSTTOKEN || n >= LASTTOKEN || proctab[n-FIRSTTOKEN]== nullproc)
#define isexpr(n)	(n->ntype == NEXPR)
#define isjump(n)	(n.otype == OJUMP)
#define isexit(n)	(n.otype == OJUMP && n.osub == JEXIT)
#define	isbreak(n)	(n.otype == OJUMP && n.osub == JBREAK)
#define	iscont(n)	(n.otype == OJUMP && n.osub == JCONT)
#define	isnext(n)	(n.otype == OJUMP && n.osub == JNEXT)
#define isstr(n)	(n.optr->tval & STR)
#define istrue(n)	(n.otype == OBOOL && n.osub == BTRUE)
#define istemp(n)	(n.otype == OCELL && n.osub == CTEMP)
#define isfld(n)	(!donefld && n.osub==CFLD && n.otype==OCELL && n.optr->nval==EMPTY)
#define isrec(n)	(donefld && n.osub==CFLD && n.otype==OCELL && n.optr->nval!=EMPTY)
obj	nullproc(void);
obj	relop(node **a, int n);

#define MAXSYM	50
#define	HAT	0177	/* matches ^ in regular expr */
			/* watch out for mach dep */
struct fa;
void yyerror(char *s);
void error(int f, char *s, ...);
void syminit(void);
int freeze(char *s);
int thaw(char *s);
void run(void);
void recbld(void);
int isnumber(char *s);
int getrec(void);
int member(char c, char *s);
void fldbld(void);
void freesymtab(cell *ap);
int match(struct fa *pfa, char *p);
