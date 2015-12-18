/* This file is in the public domain. */

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifdef	maindef
/*
 * for MAIN.C
 * initialized global definitions
 */

short kbdm[NKBDM] = {CTLX | ')'}; /* Macro */
int fillcol = 72;		/* Current fill column */
char pat[NPAT];			/* Search pattern */
char rpat[NPAT];		/* replacement pattern */
int revexist = FALSE;
int eolexist = TRUE;		/* does clear to EOL exist */
int sgarbf = TRUE;		/* TRUE if screen is garbage */
int mpresf = FALSE;		/* TRUE if message in last line */

/* uninitialized global definitions */

int currow;			/* Cursor row */
int curcol;			/* Cursor column */
int thisflag;			/* Flags, this command */
int lastflag;			/* Flags, last command */
int curgoal;			/* Goal for C-P, C-N */
int rows;			/* Screen rows for C-V, M-V */
WINDOW *curwp;			/* Current window */
BUFFER *curbp;			/* Current buffer */
WINDOW *wheadp;			/* Head of list of windows */
BUFFER *bheadp;			/* Head of list of buffers */
short *kbdmip;			/* Input pointer for above */
short *kbdmop;			/* Output pointer for above */

#else
/*
 * for all the other .C files
 * initialized global external declarations
 */

extern int fillcol;		/* Fill column */
extern short kbdm[];		/* Holds kayboard macro data */
extern char pat[];		/* Search pattern */
extern char rpat[];		/* Replacement pattern */
extern int eolexist;		/* does clear to EOL exist? */
extern int revexist;		/* does reverse video exist? */
extern char *modename[];	/* text names of modes */
extern char modecode[];		/* letters to represent modes */
extern KEYTAB keytab[];		/* key bind to functions table */
extern int gmode;		/* global editor mode */
extern int sgarbf;		/* State of screen unknown */
extern int mpresf;		/* Stuff in message line */
extern int clexec;		/* command line execution flag */

/* initialized global external declarations */

extern int currow;		/* Cursor row */
extern int curcol;		/* Cursor column */
extern int thisflag;		/* Flags, this command */
extern int lastflag;		/* Flags, last command */
extern int curgoal;		/* Goal for C-P, C-N */
extern int rows;		/* Screen rows for C-V, M-V */
extern WINDOW *curwp;		/* Current window */
extern BUFFER *curbp;		/* Current buffer */
extern WINDOW *wheadp;		/* Head of list of windows */
extern BUFFER *bheadp;		/* Head of list of buffers */
extern short *kbdmip;		/* Input pointer for above */
extern short *kbdmop;		/* Output pointer for above */

#endif

extern TERM term;		/* Terminal information */
