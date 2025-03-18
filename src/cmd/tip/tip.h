/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *  @(#)tip.h   5.3.1 (2.11BSD GTE) 1/1/94
 */

/*
 * tip - terminal interface program
 */

#include <sys/types.h>
#include <sys/file.h>

#include <sgtty.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <ctype.h>
#include <setjmp.h>
#include <errno.h>

#ifdef CTRL
#undef CTRL
#endif
#define CTRL(c) ('c' & 037)

/*
 * Remote host attributes
 */
extern char *DV;        /* UNIX device(s) to open */
extern char *EL;        /* chars marking an EOL */
extern char *CM;        /* initial connection message */
extern char *IE;        /* EOT to expect on input */
extern char *OE;        /* EOT to send to complete FT */
extern char *CU;        /* call unit if making a phone call */
extern char *AT;        /* acu type */
extern char *PN;        /* phone number(s) */
extern char *DI;        /* disconnect string */
extern char *PA;        /* parity to be generated */

extern char *PH;        /* phone number file */
extern char *RM;        /* remote file name */
extern char *HO;        /* host name */

extern int BR;          /* line speed for conversation */
extern int FS;          /* frame size for transfers */

extern char DU;         /* this host is dialed up */
extern char HW;         /* this device is hardwired, see hunt.c */
extern char *ES;        /* escape character */
extern char *EX;        /* exceptions */
extern char *FO;        /* force (literal next) char*/
extern char *RC;        /* raise character */
extern char *RE;        /* script record file */
extern char *PR;        /* remote prompt */
extern int DL;          /* line delay for file transfers to remote */
extern int CL;          /* char delay for file transfers to remote */
extern int ET;          /* echocheck timeout */
extern char HD;         /* this host is half duplex - do local echo */

/*
 * String value table
 */
typedef struct {
    char    *v_name;    /* whose name is it */
    char    v_type;     /* for interpreting set's */
    char    v_access;   /* protection of touchy ones */
    char    *v_abrev;   /* possible abreviation */
    char    *v_value;   /* casted to a union later */
} value_t;

#define STRING  01      /* string valued */
#define BOOL    02      /* true-false value */
#define NUMBER  04      /* numeric value */
#define CHAR    010     /* character value */

#define WRITE   01      /* write access to variable */
#define READ    02      /* read access */

#define CHANGED 01      /* low bit is used to show modification */
#define PUBLIC  1       /* public access rights */
#define PRIVATE 03      /* private to definer */
#define ROOT    05      /* root defined */

#define TRUE    1
#define FALSE   0

#define ENVIRON 020     /* initialize out of the environment */
#define IREMOTE 040     /* initialize out of remote structure */
#define INIT    0100    /* static data space used for initialization */
#define TMASK   017

/*
 * Definition of ACU line description
 */
typedef struct {
    char    *acu_name;
    int (*acu_dialer)(char*, char*);
    int (*acu_disconnect)();
    int (*acu_abort)();
} acu_t;

#define equal(a, b) (strcmp(a,b)==0)/* A nice function to string compare */

/*
 * variable manipulation stuff --
 *   if we defined the value entry in value_t, then we couldn't
 *   initialize it in vars.c, so we cast it as needed to keep lint
 *   happy.
 */
typedef union {
    int zz_number;
    short   zz_boolean;
    char    zz_character;
    int *zz_address;
} zzhack;

#define value(v)        vtable[v].v_value

#define boolean(v)      ((((zzhack *)(&(v))))->zz_boolean)
#define number(v)       ((((zzhack *)(&(v))))->zz_number)
#define character(v)    ((((zzhack *)(&(v))))->zz_character)
#define address(v)      ((((zzhack *)(&(v))))->zz_address)

/*
 * Escape command table definitions --
 *   lookup in this table is performed when ``escapec'' is recognized
 *   at the begining of a line (as defined by the eolmarks variable).
*/

typedef struct {
    char    e_char;         /* char to match on */
    char    e_flags;        /* experimental, priviledged */
    char    *e_help;        /* help string */
    int     (*e_func)(int);    /* command */
} esctable_t;

#define NORM    00          /* normal protection, execute anyone */
#define EXP     01          /* experimental, mark it with a `*' on help */
#define PRIV    02          /* priviledged, root execute only */

extern int      vflag;      /* verbose during reading of .tiprc file */
extern value_t  vtable[];   /* variable table */

/*
 * Definition of indices into variable table so
 *  value(DEFINE) turns into a static address.
 */

#define BEAUTIFY    0
#define BAUDRATE    1
#define DIALTIMEOUT 2
#define EOFREAD     3
#define EOFWRITE    4
#define EOL         5
#define ESCAPE      6
#define EXCEPTIONS  7
#define FORCE       8
#define FRAMESIZE   9
#define HOST        10
#define LOG         11
#define PHONES      12
#define PROMPT      13
#define RAISE       14
#define RAISECHAR   15
#define RECORD      16
#define REMOTE      17
#define SCRIPT      18
#define TABEXPAND   19
#define VERBOSE     20
#define SHELL       21
#define HOME        22
#define ECHOCHECK   23
#define DISCONNECT  24
#define TAND        25
#define LDELAY      26
#define CDELAY      27
#define ETIMEOUT    28
#define RAWFTP      29
#define HALFDUPLEX  30
#define LECHO       31
#define PARITY      32

#define NOVAL   ((value_t *)NULL)
#define NOACU   ((acu_t *)NULL)
#define NOSTR   ((char *)NULL)
#define NOFILE  ((FILE *)NULL)
#define NOPWD   ((struct passwd *)0)

extern struct sgttyb   arg;        /* current mode of local terminal */
extern struct sgttyb   defarg;     /* initial mode of local terminal */
extern struct tchars   tchars;     /* current state of terminal */
extern struct tchars   defchars;   /* initial state of terminal */
extern struct ltchars  ltchars;    /* current local characters of terminal */
extern struct ltchars  deflchars;  /* initial local characters of terminal */

extern FILE    *fscript;    /* FILE for scripting */

extern int fildes[2];       /* file transfer synchronization channel */
extern int repdes[2];       /* read process sychronization channel */
extern int FD;              /* open file descriptor to remote host */
extern int AC;              /* open file descriptor to dialer (v831 only) */
extern int vflag;           /* print .tiprc initialization sequence */
extern int sfd;             /* for ~< operation */
extern int pid;             /* pid of tipout */
extern uid_t uid, euid;     /* real and effective user id's */
extern gid_t gid, egid;     /* real and effective group id's */
extern int stop;            /* stop transfer session flag */
extern int quit;            /* same; but on other end */
extern int intflag;         /* recognized interrupt */
extern int stoprompt;       /* for interrupting a prompt session */
extern int timedout;        /* ~> transfer timedout */
extern int cumode;          /* simulating the "cu" program */

extern char fname[80];      /* file name buffer for ~< */
extern char copyname[80];   /* file name buffer for ~> */
extern char ccc;            /* synchronization character */
extern char ch;             /* for tipout */
extern char *uucplock;      /* name of lock file for uucp's */

extern int odisc;           /* initial tty line discipline */
extern int disc;            /* current tty discpline */

char *ctrl (int c);
char *connect (void);
void delock (char *s);
void pwrite (int fd, char *buf, int n);
int size (char *s);
void disconnect (char *reason);
void loginit (void);
void logent (char *group, char *num, char *acu, char *message);
int any (int c, char *p);
void raw (void);
void unraw (void);
int prompt (char *s, char *p);
void user_uid (void);
void shell_uid (void);
void daemon_uid (void);
void vinit (void);
void vlex (char *s);
int vstring (char *s, char *v);
void setparity (char *defparity);
int speed (int n);
int hunt (char *name);
void ttysetup (int speed);
int mlock (char *sys);
int rgetent (char *bp, char *name);
char *rgetstr (char *id, char **area);
int rgetnum (char *id);
int rgetflag (char *id);
void cumain (int argc, char *argv[]);
void tipout (void);
void setscript (void);
int escape (void);
void tipabort (char *msg);
void finish (void);
char * expand(char name[]);

#ifndef ACULOG
#define logent(a, b, c, d)
#define loginit()
#endif
