/*
 * UNIX shell
 */
typedef int BOOL;

#define uchar unsigned char

#define BYTESPERWORD    (sizeof (char *))
#define NIL     	0

/*
 * the following nonsense is required
 * because casts turn an Lvalue
 * into an Rvalue so two cheats
 * are necessary, one for each context.
 */
#define Lcheat(a)	(*(int*)&(a))
#define Rcheat(a)       ((int)(a))


/* address puns for storage allocation */
typedef union
{
	struct forknod  *_forkptr;
	struct comnod   *_comptr;
	struct fndnod   *_fndptr;
	struct parnod   *_parptr;
	struct ifnod    *_ifptr;
	struct whnod    *_whptr;
	struct fornod   *_forptr;
	struct lstnod   *_lstptr;
	struct blk      *_blkptr;
	struct namnod   *_namptr;
	char    *_bytptr;
} address;


/* heap storage */
struct blk
{
	struct blk      *word;
};

#define BUFSIZ  128
struct fileblk
{
	int     fdes;
	unsigned flin;
	BOOL    feof;
	uchar    fsiz;
	char    *fnxt;
	char    *fend;
	char    **feval;
	struct fileblk  *fstak;
	char    fbuf[BUFSIZ];
};

struct tempblk
{
	int fdes;
	struct tempblk *Fstak;
};


/* for files not used with file descriptors */
struct filehdr
{
	int     fdes;
	unsigned        flin;
	BOOL    feof;
	uchar    fsiz;
	char    *fnxt;
	char    *fend;
	char    **feval;
	struct fileblk  *fstak;
	char    _fbuf[1];
};

struct sysnod
{
	char    *sysnam;
	int     sysval;
};

/* this node is a proforma for those that follow */
struct trenod
{
	int     tretyp;
	struct ionod    *treio;
};

/* dummy for access only */
struct argnod
{
	struct argnod   *argnxt;
	char    argval[1];
};

struct dolnod
{
	struct dolnod   *dolnxt;
	int     doluse;
	char    *dolarg[1];
};

struct forknod
{
	int     forktyp;
	struct ionod    *forkio;
	struct trenod   *forktre;
};

struct comnod
{
	int     comtyp;
	struct ionod    *comio;
	struct argnod   *comarg;
	struct argnod   *comset;
};

struct fndnod
{
	int     fndtyp;
	char    *fndnam;
	struct trenod   *fndval;
};

struct ifnod
{
	int     iftyp;
	struct trenod   *iftre;
	struct trenod   *thtre;
	struct trenod   *eltre;
};

struct whnod
{
	int     whtyp;
	struct trenod   *whtre;
	struct trenod   *dotre;
};

struct fornod
{
	int     fortyp;
	struct trenod   *fortre;
	char    *fornam;
	struct comnod   *forlst;
};

struct swnod
{
	int     swtyp;
	char *swarg;
	struct regnod   *swlst;
};

struct regnod
{
	struct argnod   *regptr;
	struct trenod   *regcom;
	struct regnod   *regnxt;
};

struct parnod
{
	int     partyp;
	struct trenod   *partre;
};

struct lstnod
{
	int     lsttyp;
	struct trenod   *lstlef;
	struct trenod   *lstrit;
};

struct ionod
{
	int     iofile;
	char    *ioname;
	char    *iolink;
	struct ionod    *ionxt;
	struct ionod    *iolst;
};

struct fdsave
{
	int org_fd;
	int dup_fd;
};


#define fndptr(x)       ((struct fndnod *)x)
#define comptr(x)       ((struct comnod *)x)
#define forkptr(x)      ((struct forknod *)x)
#define parptr(x)       ((struct parnod *)x)
#define lstptr(x)       ((struct lstnod *)x)
#define forptr(x)       ((struct fornod *)x)
#define whptr(x)        ((struct whnod *)x)
#define ifptr(x)        ((struct ifnod *)x)
#define swptr(x)        ((struct swnod *)x)
