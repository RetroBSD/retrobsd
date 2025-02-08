/*
 * UNIX shell
 */

/* error exits from various parts of shell */
#define ERROR           1
#define SYNBAD          2
#define SIGFAIL         2000
#define SIGFLG          0200

/* command tree */
#define FPRS            0x0100
#define FINT            0x0200
#define FAMP            0x0400
#define FPIN            0x0800
#define FPOU            0x1000
#define FPCL            0x2000
#define FCMD            0x4000
#define COMMSK          0x00F0
#define CNTMSK          0x000F

#define TCOM            0x0000
#define TPAR            0x0010
#define TFIL            0x0020
#define TLST            0x0030
#define TIF             0x0040
#define TWH             0x0050
#define TUN             0x0060
#define TSW             0x0070
#define TAND            0x0080
#define TORF            0x0090
#define TFORK           0x00A0
#define TFOR            0x00B0
#define TFND            0x00C0

/* execute table */
#define SYSSET          1
#define SYSCD           2
#define SYSEXEC         3

#define SYSLOGIN        4
#define SYSNEWGRP       29

#define SYSTRAP         5
#define SYSEXIT         6
#define SYSSHFT         7
#define SYSWAIT         8
#define SYSCONT         9
#define SYSBREAK        10
#define SYSEVAL         11
#define SYSDOT          12
#define SYSRDONLY       13
#define SYSTIMES        14
#define SYSXPORT        15
#define SYSNULL         16
#define SYSREAD         17
#define SYSTST          18

#ifndef RES     /*      exclude umask code      */
#define SYSUMASK        20
#define SYSULIMIT       21
#endif

#define SYSECHO         22
#define SYSHASH         23
#define SYSPWD          24
#define SYSRETURN       25
#define SYSUNS          26
#define SYSMEM          27
#define SYSTYPE         28

/* used for input and output of shell */
#define INIO            19
#define OTIO            18

/*io nodes*/
#define USERIO          10
#define IOUFD           15
#define IODOC           16
#define IOPUT           32
#define IOAPP           64
#define IOMOV           128
#define IORDW           256
#define IOSTRIP         512
#define INPIPE          0
#define OTPIPE          1

/* arg list terminator */
#define ENDARGS         0

#include "mac.h"
#include "mode.h"
#include "name.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

/*      error catching */
extern int              errno;

/* result type declarations */

#define free            afree

#define attrib(n,f)             (n->namflg |= f)
#define round(a,b)              (((int)(((char *)(a)+b)-1))&~((b)-1))
#define closepipe(x)    (close(x[INPIPE]), close(x[OTPIPE]))
#define eq(a,b)                 (cf(a,b)==0)
#define max(a,b)                ((a)>(b)?(a):(b))
#define assert(x)               ;

/* temp files and io */
extern int                              output;
extern int                              ioset;
extern struct ionod             *iotemp;        /* files to be deleted sometime */
extern struct ionod             *fiotemp;       /* function files to be deleted sometime */
extern struct ionod             *iopend;        /* documents waiting to be read at NL */
extern struct fdsave    fdmap[];


/* substitution */
extern int                              dolc;
extern char                             **dolv;
extern struct dolnod    *argfor;
extern struct argnod    *gchain;

/* stak stuff */
#include "stak.h"

/* string constants */
extern char                             atline[];
extern char                             readmsg[];
extern char                             colon[];
extern char                             minus[];
extern char                             nullstr[];
extern char                             sptbnl[];
extern char                             unexpected[];
extern char                             endoffile[];
extern char                             synmsg[];

/* name tree and words */
extern struct sysnod    reserved[];
extern int                              no_reserved;
extern struct sysnod    commands[];
extern int                              no_commands;

extern int                              wdval;
extern int                              wdnum;
extern int                              fndef;
extern int                              nohash;
extern struct argnod    *wdarg;
extern int                              wdset;
extern BOOL                             reserv;

/* prompting */
extern char                             stdprompt[];
extern char                             supprompt[];
extern char                             profile[];
extern char                             sysprofile[];

/* built in names */
extern struct namnod    fngnod;
extern struct namnod    cdpnod;
extern struct namnod    ifsnod;
extern struct namnod    homenod;
extern struct namnod    mailnod;
extern struct namnod    pathnod;
extern struct namnod    ps1nod;
extern struct namnod    ps2nod;
extern struct namnod    mchknod;
extern struct namnod    acctnod;
extern struct namnod    mailpnod;

/* special names */
extern char                             flagadr[];
extern char                             *pcsadr;
extern char                             *pidadr;
extern char                             *cmdadr;

extern char                             defpath[];

/* names always present */
extern char                             mailname[];
extern char                             homename[];
extern char                             pathname[];
extern char                             cdpname[];
extern char                             ifsname[];
extern char                             ps1name[];
extern char                             ps2name[];
extern char                             mchkname[];
extern char                             acctname[];
extern char                             mailpname[];

/* transput */
extern char                             tmpout[];
extern char                             *tmpnam;
extern int                              serial;

#define TMPNAM          7

extern struct fileblk   *standin;

#define input           (standin->fdes)
#define eof                     (standin->feof)

extern int                              peekc;
extern int                              peekn;
extern char                             *comdiv;
extern char                             devnull[];

/* flags */
#define noexec          01
#define sysflg          01
#define intflg          02
#define prompt          04
#define setflg          010
#define errflg          020
#define ttyflg          040
#define forked          0100
#define oneflg          0200
#define rshflg          0400
#define waiting         01000
#define stdflg          02000
#define STDFLG          's'
#define execpr          04000
#define readpr          010000
#define keyflg          020000
#define hashflg         040000
#define nofngflg        0200000
#define exportflg       0400000

extern long                             flags;
extern int                              rwait;  /* flags read waiting */

/* error exits from various parts of shell */
#include <setjmp.h>
extern jmp_buf                  subshell;
extern jmp_buf                  errshell;

/* fault handling */
#include "brkincr.h"

extern unsigned                 brkincr;

#define MINTRAP         0
#define MAXTRAP         26

#define TRAPSET         2
#define SIGSET          4
#define SIGMOD          8
#define SIGCAUGHT	16

extern BOOL				trapnote;
extern char				*trapcom[];
extern BOOL				trapflg[];

/* name tree and words */
extern char				**environ;
extern char				numbuf[];
extern char				export[];
extern char				duperr[];
extern char				readonly[];

/* execflgs */
extern int				exitval;
extern int				retval;
extern BOOL				execbrk;
extern int				loopcnt;
extern int				breakcnt;
extern int				funcnt;

/* messages */
extern char				mailmsg[];
extern char				coredump[];
extern char				badopt[];
extern char				badparam[];
extern char				unset[];
extern char				badsub[];
extern char				nospace[];
extern char				nostack[];
extern char				notfound[];
extern char				badtrap[];
extern char				baddir[];
extern char				badshift[];
extern char				restricted[];
extern char				execpmsg[];
extern char				notid[];
extern char 			badulimit[];
extern char				wtfailed[];
extern char				badcreate[];
extern char				nofork[];
extern char				noswap[];
extern char				piperr[];
extern char				badopen[];
extern char				badnum[];
extern char				arglist[];
extern char				txtbsy[];
extern char				toobig[];
extern char				badexec[];
extern char				badfile[];
extern char				badreturn[];
extern char				badexport[];
extern char				badunset[];
extern char				nohome[];
extern char				badperm[];

/*	'builtin' error messages	*/

extern char				btest[];
extern char				badop[];

/*	fork constant	*/

#define FORKLIM 2               /* was 32: max retry timeout for fork() */

#include "ctype.h"

extern int				wasintr;	/* used to tell if break or delete is hit
							 *  while executing a wait */
extern int				eflag;


/*
 * Find out if it is time to go away.
 * `trapnote' is set to SIGSET when fault is seen and
 * no trap has been set.
 */

#define	sigchk()	if (trapnote & SIGSET)	\
                                exitsh(exitval ? exitval : SIGFAIL)

#define exitset()	retval = exitval

#define ENDPATH 27      /* see msg.c/defpath */

void addblok(unsigned reqd);
char *movstr(char *a, char *b);
char *setbrk(int incr);
void error(char *s);
char *alloc(unsigned nbytes);
void free(void *ap);
void rmtemp(struct ionod *base);
void prs(char *as);
void prc(char c);
void prp(void);
void prn(int n);
void prs_cntl(char *s);
void exitsh(int xno);
int word(void);
void chkpr(void);
int length(char *as);
struct trenod *cmd(int sym, int flg);
int nextc(char quote);
char *make(char *v);
struct trenod *makefork(int flgs, struct trenod *i);
int skipc(void);
short pathlook(char *com, int flg, struct argnod *arg);
void done(void);
void clrsig(int i);
void failed(char *s1, char *s2);
void execexp(char *s, void *f);
void dfault(struct namnod *n, char *v);
int ignsig(int n);
void setmail(char *mailpath);
void initf(int fd);
void tdystak(char *x);
void stakchk(void);
void prprompt(char *as);
int readc(void);
int execute(struct trenod *argt, int exec_link, int errorflg, int *pf1, int *pf2);
void stdsigs(void);
void setup_env(void);
struct namnod *findnam(char *nam);
int any(char c, char *s);
char *simple(char *s);
void hcreate(void);
void set_dotpath(void);
int options(int argc, char **argv);
void replace(char **a, char *v);
void assnum(char **p, int i);
void settmp(void);
int stoi(char *icp);
int pathopen(char *path, char *name);
int estabf(char *s);
int chkopen(char *idf);
void itos(int n);
int syslook(char *w, struct sysnod syswds[], int n);
void copy(struct ionod *ioparg);
void chktrap(void);
void clearup(void);
int cf(char *s1, char *s2);
char *mactrim(char *s);
struct namnod *lookup(char *nam);
void assign(struct namnod *n, char *v);
void func_unhash(char *name);
void freefunc(struct namnod *n);
void zaphash(void);
void push(struct fileblk *af);
int pop(void);
void prs_buff(char *s);
void prc_buff(char c);
void prf(struct trenod *t);
void namscan(void (*fn)(struct namnod *));
int poptemp(void);
void hash_func(char *name);
int getarg(struct comnod *ac);
char **scan(int argn);
void setlist(struct argnod *arg, int xp);
int initio(struct ionod *iop, int save);
char *getpath(char *s);
void prt(long t);
void getsig(int n);
void prn_buff(int n);
void oldsigs(void);
void execa(char *at[], short pos);
char *catpath(char *path, char *name);
void cwd(char *dir);
void zapcd(void);
void await(int i, int bckg);
int readvar(char **names);
void setargs(char *argi[]);
void printnam(struct namnod *n);
void printro(struct namnod *n);
void printexp(struct namnod *n);
int test(int argn, char *com[]);
int echo(int argc, char **argv);
short hash_cmd(char *name);
void hashpr(void);
void cwdprint(void);
void what_is_path(char *name);
void unset_name(char *name);
void flushb(void);
void restore(int last);
void link_iodocs(struct ionod *i);
void post(int pcsid);
void swap_iodoc_nm(struct ionod *i);
void postclr(void);
void rename(int f1, int f2);
void chkpipe(int *pv);
struct dolnod *useargs(void);
void trim(char *at);
struct dolnod *freeargs(struct dolnod *blk);
int gmatch(char *s, char *p);
char *macro(char *as);
int savefd(int fd);
void subst(int in, int ot);
int tmpfil(struct tempblk *tb);
int create(char *s);
char **setenvv(void);
int expand(char *as, int rcnt);
void makearg(struct argnod *args);
void clear_buff(void);
char *movstrn(char *a, char *b, int n);
void reset_dir(void);
void hscan(void (*uscan)());
char *nextpath(char *path);
struct direct *getdir(int dirf);
