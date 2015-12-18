/* This file is in the public domain. */

/*
 * This program is in public domain; originally written by Dave G. Conroy.
 * This file contains the main driving routine, and some keyboard processing
 * code
 */

#define maindef			/* make global definitions not external */

#include <stdio.h>
#include <stdlib.h>		/* malloc(3) */
#include <string.h>		/* strncpy(3) */
#include "estruct.h"		/* global structures and defines */
#include "efunc.h"		/* function declarations and name table */
#include "edef.h"		/* global definitions */
#include "ebind.h"

extern void getwinsize();
extern void vtinit();
extern void vttidy();
extern void update();
extern void mlerase();
extern void mlwrite();
extern int mlyesno(char *);
extern int readin(char []);
extern int linsert(int, int);
extern int anycb(void);
extern BUFFER *bfind(char *);

void edinit(char []);
int execute(int, int, int);
int getkey(void);
int getctl(void);
int quit(int, int);
int ctlxlp(int, int);
int ctlxrp(int, int);
int ctlxe(int, int);
int ctrlg(int, int);

int
main(int argc, char *argv[])
{
  BUFFER *bp;
  char fname[NFILEN];
  int c, f, n, mflag;
  int basec;			/* c stripped of meta character */

  /* In place of getopt() */
  if (argc > 2) {
    (void) fprintf(stderr, "usage: emg [file]\n");
    exit(1);
  } else if (argc == 2) {
    strncpy(fname, argv[1], NFILEN);
  } else {
    strncpy(fname, "*scratch*", NFILEN);
  }

  /* initialize the editor and process the startup file */
  getwinsize();			/* Find out the "real" screen size */
  edinit(fname);		/* Buffers, windows */
  vtinit();			/* Displays */
  update();			/* Let the user know we are here */

  /* Read in the file given on the command line */
  if (argc == 2) {
    bp = curbp;
    if (readin(argv[1]) == ABORT)
      strncpy(fname, "*scratch*", NFILEN);
    bp->b_dotp = bp->b_linep;
    bp->b_doto = 0;
  }

  /* setup to process commands */
  lastflag = 0;			/* Fake last flags */
  curwp->w_flag |= WFMODE;	/* and force an update */

loop:
  update();			/* Fix up the screen */
  c = getkey();
  if (mpresf != FALSE) {
    mlerase();
    update();
  }
  f = FALSE;
  n = 1;

  /* do META-# processing if needed */

  basec = c & ~META;		/* strip meta char off if there */
  if ((c & META) && ((basec >= '0' && basec <= '9') || basec == '-')) {
    f = TRUE;			/* there is a # arg */
    n = 0;			/* start with a zero default */
    mflag = 1;		/* current minus flag */
    c = basec;		/* strip the META */
    while ((c >= '0' && c <= '9') || (c == '-')) {
      if (c == '-') {
	/* already hit a minus or digit? */
	if ((mflag == -1) || (n != 0))
	  break;
	mflag = -1;
      } else
	n = n * 10 + (c - '0');
      if ((n == 0) && (mflag == -1)) /* lonely - */
	mlwrite("Arg:");
      else
	mlwrite("Arg: %d", n * mflag);

      c = getkey();		/* get the next key */
    }
    n = n * mflag;		/* figure in the sign */
  }

  /* do ^U repeat argument processing */
  if (c == (CTRL | 'U')) {	/* ^U, start argument */
    f = TRUE;
    n = 4;			/* with argument of 4 */
    mflag = 0;		 /* that can be discarded */
    mlwrite("Arg: 4");
    while (((c = getkey()) >= '0')
	    && ((c <= '9') || (c == (CTRL | 'U')) || (c == '-'))) {
      if (c == (CTRL | 'U'))
	n = n * 4;
      /*
       * If dash, and start of argument string, set arg.
       * to -1.  Otherwise, insert it.
       */
      else if (c == '-') {
	if (mflag)
	  break;
	n = 0;
	mflag = -1;
      /*
       * If first digit entered, replace previous argument
       * with digit and set sign.  Otherwise, append to arg.
       */
      } else {
	if (!mflag) {
	  n = 0;
	  mflag = 1;
	}
	n = 10 * n + c - '0';
      }
      mlwrite("Arg: %d", (mflag >= 0) ? n : (n ? -n : -1));
    }
    /*
     * Make arguments preceded by a minus sign negative and change
     * the special argument "^U -" to an effective "^U -1".
     */
    if (mflag == -1) {
      if (n == 0)
	n++;
      n = -n;
    }
  }

  if (c == (META | '['))	/* M-[ is extended ANSI sequence */
    c = METE | getctl();
  else if (c == (CTRL | 'X'))	/* ^X is a prefix */
    c = CTLX | getctl();

  if (kbdmip != NULL) {		/* Save macro strokes */
    if (c != (CTLX | ')') && kbdmip > &kbdm[NKBDM - 6]) {
      ctrlg(FALSE, 0);
      goto loop;
    }
    if (f != FALSE) {
      *kbdmip++ = (CTRL | 'U');
      *kbdmip++ = n;
    }
    *kbdmip++ = c;
  }
  execute(c, f, n);	       /* Do it */
  goto loop;
}

/*
 * Initialize all of the buffers and windows. The buffer name is passed down
 * as an argument, because the main routine may have been told to read in a
 * file by default, and we want the buffer name to be right.
 */
void
edinit(char fname[])
{
  BUFFER *bp;
  WINDOW *wp;

  bp = bfind(fname);
  wp = (WINDOW *) malloc(sizeof(WINDOW));
  if (bp == NULL || wp == NULL)
    exit(1);
  curbp = bp;			/* Make this current */
  wheadp = wp;
  curwp = wp;
  wp->w_wndp = NULL;		/* Initialize window */
  wp->w_bufp = bp;
  wp->w_linep = bp->b_linep;
  wp->w_dotp = bp->b_linep;
  wp->w_doto = 0;
  wp->w_markp = NULL;
  wp->w_marko = 0;
  wp->w_toprow = 0;
  wp->w_ntrows = term.t_nrow - 1; /* "-1" for mode line */
  wp->w_force = 0;
  wp->w_flag = WFMODE | WFHARD;	/* Full */
}

/*
 * This is the general command execution routine. It handles the fake binding
 * of all the keys to "self-insert". It also clears out the "thisflag" word,
 * and arranges to move it to the "lastflag", so that the next command can
 * look at it. Return the status of command.
 */
int
execute(int c, int f, int n)
{
  KEYTAB *ktp;
  int status;

  ktp = &keytab[0];	       /* Look in key table */
  while (ktp->k_fp != NULL) {
    if (ktp->k_code == c) {
      thisflag = 0;
      status = (*ktp->k_fp) (f, n);
      lastflag = thisflag;
      return (status);
    }
    ++ktp;
  }

  if ((c >= 0x20 && c <= 0x7E)	/* Self inserting */
      || (c >= 0xA0 && c <= 0xFE)) {
    if (n <= 0) {		/* Fenceposts */
      lastflag = 0;
      return (n < 0 ? FALSE : TRUE);
    }
    thisflag = 0;		/* For the future */

    status = linsert(n, c);

    lastflag = thisflag;
    return (status);
  }
  mlwrite("\007[Key not bound]"); /* complain */
  lastflag = 0;			/* Fake last flags */
  return (FALSE);
}

/*
 * Read in a key. Do the standard keyboard preprocessing. Convert the keys to
 * the internal character set.
 */
int
getkey(void)
{
  int c;

  c = (*term.t_getchar) ();

  if (c == METACH) {		/* Apply M- prefix */
    c = getctl();
    return (META | c);
  }
  if (c >= 0x00 && c <= 0x1F)	/* C0 control -> C- */
    c = CTRL | (c + '@');
  return (c);
}

/*
 * Get a key. Apply control modifications to the read key.
 */
int
getctl()
{
  int c;

  c = (*term.t_getchar)();
  if (c >= 'a' && c <= 'z')	/* Force to upper */
    c -= 0x20;
  if (c >= 0x00 && c <= 0x1F)	/* C0 control -> C- */
    c = CTRL | (c + '@');
  return (c);
}

/*
 * Quit command. If an argument, always quit. Otherwise confirm if a buffer
 * has been changed and not written out. Normally bound to "C-X C-C".
 */
int
quit(int f, int n)
{
  int s;

  if (f != FALSE	       /* Argument forces it */
      || anycb() == FALSE     /* All buffers clean */
      || (s = mlyesno("Quit without saving")) == TRUE) {
    vttidy();
    exit(0);
  }
  mlwrite("");
  return (s);
}

/*
 * Begin a keyboard macro. Error if not at the top level in keyboard
 * processing. Set up variables and return.
 */
int
ctlxlp(int f, int n)
{
  if (kbdmip != NULL || kbdmop != NULL) {
    mlwrite("Not now");
    return (FALSE);
  }
  mlwrite("[Start macro]");
  kbdmip = &kbdm[0];
  return (TRUE);
}

/*
 * End keyboard macro. Check for the same limit conditions as the above
 * routine. Set up the variables and return to the caller.
 */
int
ctlxrp(int f, int n)
{
  if (kbdmip == NULL) {
    mlwrite("Not now");
    return (FALSE);
  }
  mlwrite("[End macro]");
  kbdmip = NULL;
  return (TRUE);
}

/*
 * Execute a macro. The command argument is the number of times to loop. Quit
 * as soon as a command gets an error. Return TRUE if all ok, else FALSE.
 */
int
ctlxe(int f, int n)
{
  int c, af, an, s;

  if (kbdmip != NULL || kbdmop != NULL) {
    mlwrite("No macro defined");
    return (FALSE);
  }
  if (n <= 0)
    return (TRUE);
  do {
    kbdmop = &kbdm[0];
    do {
      af = FALSE;
      an = 1;
      if ((c = *kbdmop++) == (CTRL | 'U')) {
	af = TRUE;
	an = *kbdmop++;
	c = *kbdmop++;
      }
      s = TRUE;
    } while (c != (CTLX | ')') && (s = execute(c, af, an)) == TRUE);
    kbdmop = NULL;
  } while (s == TRUE && --n);
  return (s);
}

/*
 * Abort. Beep the beeper. Kill off any keyboard macro, etc., that is in
 * progress. Sometimes called as a routine, to do general aborting of stuff.
 */
int
ctrlg(int f, int n)
{
  (*term.t_beep) ();
  if (kbdmip != NULL) {
    kbdm[0] = (CTLX | ')');
    kbdmip = NULL;
  }
  mlwrite("[Aborted]");
  return (ABORT);
}

/*
 * Display the version. All this does
 * is copy the version string into the echo line.
 * Taken from OpenBSD Mg.
 * THIS IS WHERE YOU INCREMENT VERSION NUMBER BEFORE RELEASE!
 */
int
showversion(int f, int n)
{
  mlwrite("emg 2.0");
  return (TRUE);
}
