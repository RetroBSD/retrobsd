/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#define MAXDUP      10  /* limit on dup blks (per inode) */
#define MAXBAD      10  /* limit on bad blks (per inode) */

#define STEPSIZE    7   /* default step for freelist spacing */
#define CYLSIZE     400 /* default cyl size for spacing */
#define MAXCYL      500 /* maximum cylinder size */

#ifndef BUFSIZ
#define BUFSIZ 1024
#endif

#define USTATE  01      /* inode not allocated */
#define FSTATE  02      /* inode is file */
#define DSTATE  03      /* inode is directory */
#define DFOUND  04      /* directory found during descent */
#define DCLEAR  05      /* directory is to be cleared */
#define FCLEAR  06      /* file is to be cleared */

#define BITSPB  8       /* number bits per byte */
#define BITSHIFT    3   /* log2(BITSPB) */
#define BITMASK 07      /* BITSPB-1 */
#define LSTATE  4       /* bits per inode state */
#define STATEPB (BITSPB/LSTATE) /* inode states per byte */
#define SMASK   017     /* mask for inode state */

typedef struct dinode   DINODE;
typedef struct direct   DIRECT;

#define ALLOC(dip)  (((dip)->di_mode & IFMT) != 0)
#define DIRCT(dip)  (((dip)->di_mode & IFMT) == IFDIR)
#define SPECIAL(dip) \
    (((dip)->di_mode & IFMT) == IFBLK || ((dip)->di_mode & IFMT) == IFCHR)

#define MAXNINDIR   (MAXBSIZE / sizeof (daddr_t))
#define MAXINOPB    (MAXBSIZE / sizeof (struct dinode))
#define SPERB       (MAXBSIZE / sizeof(short))

struct bufarea {
    struct bufarea  *b_next;        /* must be first */
    daddr_t b_bno;
    int b_size;
    int b_errs;
    union {
        char    b_buf[MAXBSIZE];    /* buffer space */
        short   b_lnks[SPERB];      /* link counts */
        daddr_t b_indir[MAXNINDIR]; /* indirect block */
        struct  fs b_fs;        /* super block */
        struct  fblk b_fb;      /* free block */
        struct dinode b_dinode[MAXINOPB]; /* inode block */
    } b_un;
    char    b_dirty;
};

typedef struct bufarea BUFAREA;

BUFAREA inoblk;         /* inode blocks */
BUFAREA fileblk;        /* other blks in filesys */
BUFAREA sblk;           /* file system superblock */
BUFAREA *poolhead;      /* ptr to first buffer in pool */

#define initbarea(x)    (x)->b_dirty = 0;(x)->b_bno = (daddr_t)-1
#define dirty(x)    (x)->b_dirty = 1
#define inodirty()  inoblk.b_dirty = 1
#define fbdirty()   fileblk.b_dirty = 1
#define sbdirty()   sblk.b_dirty = 1

#define dirblk      fileblk.b_un
#define freeblk     fileblk.b_un.b_fb
#define sblock      sblk.b_un.b_fs

struct filecntl {
    int rfdes;
    int wfdes;
    int mod;
    off_t   offset;
} dfile, sfile;         /* file descriptors for filesys/scratch files */

enum fixstate {DONTKNOW, NOFIX, FIX};

struct inodesc {
    enum fixstate id_fix;   /* policy on fixing errors */
    int (*id_func)();   /* function to be applied to blocks of inode */
    ino_t id_number;    /* inode number described */
    ino_t id_parent;    /* for DATA nodes, their parent */
    daddr_t id_blkno;   /* current block number being examined */
    long id_filesize;   /* for DATA nodes, the size of the directory */
    off_t id_loc;       /* for DATA nodes, current location in dir */
    u_long id_entryno;  /* for DATA nodes, current entry number */
    DIRECT *id_dirp;    /* for DATA nodes, ptr to current entry */
    char *id_name;      /* for DATA nodes, name to find or enter */
    char id_type;       /* type of descriptor, DATA or ADDR */
};
/* file types */
#define DATA    1
#define ADDR    2

/*
 * Linked list of duplicate blocks.
 *
 * The list is composed of two parts. The first part of the
 * list (from duplist through the node pointed to by muldup)
 * contains a single copy of each duplicate block that has been
 * found. The second part of the list (from muldup to the end)
 * contains duplicate blocks that have been found more than once.
 * To check if a block has been found as a duplicate it is only
 * necessary to search from duplist through muldup. To find the
 * total number of times that a block has been found as a duplicate
 * the entire list must be searched for occurences of the block
 * in question. The following diagram shows a sample list where
 * w (found twice), x (found once), y (found three times), and z
 * (found once) are duplicate block numbers:
 *
 *    w -> y -> x -> z -> y -> w -> y
 *    ^          ^
 *    |          |
 * duplist    muldup
 */

#define DUPTBLSIZE  100

daddr_t duplist[DUPTBLSIZE];    /* head of dup list */
daddr_t *enddup;            /* next entry in dup table */
daddr_t *muldup;        /* multiple dups part of table */

/*
 * List of inodes with zero link counts.
 */

#define MAXLNCNT    50

ino_t   zlnlist[MAXLNCNT];  /* zero link count table */
ino_t   *zlnp;

#define MAXDATA (90 * 1024)
#define MEMUNIT 64
#define NINOBLK 4       /* number of blocks of inodes to read at once */

char    inobuf[NINOBLK*INOPB*sizeof (struct dinode)];   /* allocate now */
daddr_t startib;

unsigned int memsize;
char    rawflg;
char    *devnam;
char    nflag;          /* assume a no response */
char    yflag;          /* assume a yes response */
char    sflag;          /* rebuild free list */
int debug;          /* output debugging info */
char    preen;          /* just fix normal inconsistencies */
char    hotroot;        /* checking root device */
char    fixfree;        /* force rebuild of freelist */
char    *membase;       /* base of memory we get */

char    *blockmap;      /* ptr to primary blk allocation map */
char    *freemap;       /* ptr to secondary blk allocation map */
char    *statemap;      /* ptr to inode state table */
short   *lncntp;        /* ptr to link count table */

char    pathname[MAXPATHLEN];   /* current pathname */
char    scrfile[80];        /* scratchfile name */
int cylsize;        /* num blocks per cylinder */
int stepsize;       /* num blocks for spacing purposes */
char    *pathp;         /* pointer to pathname position */
char    *endpathname;

daddr_t fsmin;          /* block number of the first data block */
daddr_t fsmax;          /* number of blocks in the volume */
ino_t   imax;           /* number of inodes */
ino_t   lastino;        /* hiwater mark of inodes */
ino_t   lfdir;          /* lost & found directory inode number */
char    *lfname;        /* lost & found directory name */

off_t   bmapsz;         /* num chars in blockmap */
daddr_t bmapblk;        /* starting blk of block map */
daddr_t smapblk;        /* starting blk of state map */
daddr_t lncntblk;       /* starting blk of link cnt table */
daddr_t fmapblk;        /* starting blk of free map */

daddr_t n_blks;         /* number of blocks used */
daddr_t n_files;        /* number of files seen */
daddr_t n_free;         /* number of free blocks */
int badblk, dupblk;

#define outrange(x) (x < fsmin || x >= fsmax)
#define zapino(x)   (*(x) = zino)
struct  dinode  zino;

#define setlncnt(x,n)   dolncnt(x,0,n)
#define getlncnt(x) dolncnt(x,1,0)
#define declncnt(x) dolncnt(x,2,0)
#define inclncnt(x) dolncnt(x,3,0)

#define setbmap(x)  domap(x,0)
#define getbmap(x)  domap(x,1)
#define clrbmap(x)  domap(x,2)

#define setfmap(x)  domap(x,0+4)
#define getfmap(x)  domap(x,1+4)
#define clrfmap(x)  domap(x,2+4)

#define setstate(x,y)   dostate(x,y,0)
#define getstate(x) dostate(x,0,1)

#define ALTERED 010
#define KEEPON  04
#define SKIP    02
#define STOP    01

DINODE  *ginode (ino_t);
BUFAREA *getblk (BUFAREA *, daddr_t);
daddr_t allocblk (void);
int     findino (struct inodesc *);

void    catch (int), catchquit (int), voidquit (int);

void    errexit (char *, ...);
int     setup (char *);
void    pfatal (char *, ...);
void    pwarn (char *, ...);
void    flush (struct filecntl *, BUFAREA *);
int     dostate (ino_t, int, int);
int     reply (char *);
int     dofix (struct inodesc *, char *);
int     ftypeok (DINODE *);
int     dolncnt (ino_t, short, short);
void    bwrite (struct filecntl *, char *, daddr_t, int);
int     bread (struct filecntl *, char *, daddr_t, int);
int     domap (daddr_t, int);
void    getpathname (char *, ino_t, ino_t);
int     getlin (FILE *, char *, int);

void    pass1 (void);
void    pass1b (void);
void    pass2 (void);
void    pass3 (void);
void    pass4 (void);
void    pass5 (void);
void    ckfini (void);

void    direrr (ino_t, char *);
int     ckinode (DINODE *, struct inodesc *);
void    pinode (ino_t);
int     linkup (ino_t, ino_t);
void    clri (struct inodesc *, char *, int);
int     allocdir (ino_t, ino_t);
int     pass1check (struct inodesc *);
int     pass4check (struct inodesc *);
void    freeino (ino_t);
ino_t   allocino (ino_t, int);
int     dirscan (struct inodesc *);
void    blkerr (ino_t, char *, daddr_t);
void    descend (struct inodesc *, ino_t);
int     pass2check (struct inodesc *);
void    adjust (struct inodesc *, short);
int     findname (struct inodesc *);
