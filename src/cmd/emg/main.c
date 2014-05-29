/* This file is in the public domain. */

/*
 * This program is in public domain; originally written by Dave G. Conroy.
 * This file contains the main driving routine, and some keyboard processing
 * code
 */

#define maindef			/* make global definitions not external */

#include <string.h>		/* strncpy(3) */
#include <stdlib.h>		/* malloc(3) */
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
extern int mlyesno(char *prompt);
extern void makename(char bname[], char fname[]);
extern int readin(char fname[]);
extern int linsert(int f, int n);
extern int anycb();
extern BUFFER *bfind();

void edinit(char bname[]);
int execute(int c, int f, int n);
int getkey();
int getctl();
int quickexit(int f, int n);
int quit(int f, int n);
int ctlxlp(int f, int n);
int ctlxrp(int f, int n);
int ctlxe(int f, int n);
int ctrlg(int f, int n);
int extendedcmd(int f, int n);

int main(int argc, char *argv[])
{
  BUFFER *bp;
  char bname[NBUFN];		/* buffer name of file to read */
  int c, f, n, mflag;
  int ffile;			/* first file flag */
  int carg;			/* current arg to scan */
  int basec;			/* c stripped of meta character */

  /* initialize the editor and process the startup file */
  getwinsize();			/* find out the "real" screen size */
  strncpy(bname, "main", 5);	/* default buffer name */
  edinit(bname);		/* Buffers, windows */
  vtinit();			/* Displays */
  ffile = TRUE;			/* no file to edit yet */
  update();			/* let the user know we are here */

  /* scan through the command line and get the files to edit */
  for (carg = 1; carg < argc; ++carg)
    {
      /* set up a buffer for this file */
      makename(bname, argv[carg]);

      /* if this is the first file, read it in */
      if (ffile)
	{
	  bp = curbp;
	  makename(bname, argv[carg]);
	  strncpy(bp->b_bname, bname, NBUFN);
	  strncpy(bp->b_fname, argv[carg], NFILEN);
	  if (readin(argv[carg]) == ABORT)
	    {
	      strncpy(bp->b_bname, "main", 5);
	      strncpy(bp->b_fname, "", 1);
	    }
	  bp->b_dotp = bp->b_linep;
	  bp->b_doto = 0;
	  ffile = FALSE;
	}
      else
	{
	  /* set this to inactive */
	  bp = bfind(bname, TRUE, 0);
	  strncpy(bp->b_fname, argv[carg], NFILEN);
	  bp->b_active = FALSE;
	}
    }

  /* setup to process commands */
  lastflag = 0;			/* Fake last flags */
  curwp->w_flag |= WFMODE;	/* and force an update */

 loop:
  update();			/* Fix up the screen */
  c = getkey();
  if (mpresf != FALSE)
    {
      mlerase();
      update();
    }
  f = FALSE;
  n = 1;

  /* do META-# processing if needed */

  basec = c & ~META;		/* strip meta char off if there */
  if ((c & META) && ((basec >= '0' && basec <= '9') || basec == '-'))
    {
      f = TRUE;			/* there is a # arg */
      n = 0;			/* start with a zero default */
      mflag = 1;		/* current minus flag */
      c = basec;		/* strip the META */
      while ((c >= '0' && c <= '9') || (c == '-'))
	{
	  if (c == '-')
	    {
	      /* already hit a minus or digit? */
	      if ((mflag == -1) || (n != 0))
		break;
	      mflag = -1;
	    }
	  else
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

  if (c == (CTRL | 'U'))
    {				/* ^U, start argument */
      f = TRUE;
      n = 4;			/* with argument of 4 */
      mflag = 0;		 /* that can be discarded */
      mlwrite("Arg: 4");
      while (((c = getkey ()) >= '0')
	     && ((c <= '9') || (c == (CTRL | 'U')) || (c == '-')))
	{
	  if (c == (CTRL | 'U'))
	    n = n * 4;
	  /*
	   * If dash, and start of argument string, set arg.
	   * to -1.  Otherwise, insert it.
	   */
	  else if (c == '-')
	    {
	      if (mflag)
		break;
	      n = 0;
	      mflag = -1;
	    }
	  /*
	   * If first digit entered, replace previous argument
	   * with digit and set sign.  Otherwise, append to arg.
	   */
	  else
	    {
	      if (!mflag)
		{
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
      if (mflag == -1)
	{
	  if (n == 0)
	    n++;
	  n = -n;
	}
    }

  if (c == (CTRL | 'X'))       /* ^X is a prefix */
    c = CTLX | getctl ();
  if (kbdmip != NULL)
    {				 /* Save macro strokes */
      if (c != (CTLX | ')') && kbdmip > &kbdm[NKBDM - 6])
	{
	  ctrlg(FALSE, 0);
	  goto loop;
	}
      if (f != FALSE)
	{
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
void edinit(char bname[])
{
  BUFFER *bp;
  WINDOW *wp;

  bp = bfind(bname, TRUE, 0);	/* First buffer */
  blistp = bfind("[List]", TRUE, BFTEMP); /* Buffer list buffer */
  wp = (WINDOW *) malloc(sizeof(WINDOW)); /* First window */
  if (bp == NULL || wp == NULL || blistp == NULL)
    exit (1);
  curbp = bp;			/* Make this current */
  wheadp = wp;
  curwp = wp;
  wp->w_wndp = NULL;		/* Initialize window */
  wp->w_bufp = bp;
  bp->b_nwnd = 1;		/* Displayed */
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
int execute(int c, int f, int n)
{
  KEYTAB *ktp;
  int status;

  ktp = &keytab[0];	       /* Look in key table */
  while (ktp->k_fp != NULL)
    {
      if (ktp->k_code == c)
	{
	  thisflag = 0;
	  status = (*ktp->k_fp) (f, n);
	  lastflag = thisflag;
	  return (status);
	}
      ++ktp;
    }

  if ((c >= 0x20 && c <= 0x7E)	/* Self inserting */
      || (c >= 0xA0 && c <= 0xFE))
    {
      if (n <= 0)
	{			/* Fenceposts */
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
int getkey()
{
  int c;

  c = (*term.t_getchar) ();

  if (c == METACH)
    {				/* Apply M- prefix */
      c = getctl ();
      return (META | c);
    }
  if (c >= 0x00 && c <= 0x1F)	/* C0 control -> C- */
    c = CTRL | (c + '@');
  return (c);
}

/*
 * Get a key. Apply control modifications to the read key.
 */
int getctl()
{
  int c;

  c = (*term.t_getchar) ();
  if (c >= 'a' && c <= 'z')	/* Force to upper */
    c -= 0x20;
  if (c >= 0x00 && c <= 0x1F)	/* C0 control -> C- */
    c = CTRL | (c + '@');
  return (c);
}

/*
 * Fancy quit command, as implemented by Norm. If any buffer has changed
 * do a write on that buffer and exit emacs, otherwise simply exit.
 */
int quickexit(int f, int n)
{
  BUFFER *bp;			/* scanning pointer to buffers */

  bp = bheadp;
  while (bp != NULL)
    {
      if ((bp->b_flag & BFCHG) != 0 /* Changed */
	  && (bp->b_flag & BFTEMP) == 0)
	{			/* Real */
	  curbp = bp;		/* make that buffer current */
	  mlwrite("[Saving %s]", (int*)bp->b_fname);
	  filesave(f, n);
	}
      bp = bp->b_bufp;		/* on to the next buffer */
    }
  return quit(f, n);		/* conditionally quit */
}

/*
 * Quit command. If an argument, always quit. Otherwise confirm if a buffer
 * has been changed and not written out. Normally bound to "C-X C-C".
 */
int quit(int f, int n)
{
  int s;

  if (f != FALSE	       /* Argument forces it */
      || anycb () == FALSE     /* All buffers clean */
      || (s = mlyesno("Modified buffers exist. Leave anyway")) == TRUE)
    {
      vttidy();
      exit (0);
    }
  mlwrite("");
  return (s);
}

/*
 * Begin a keyboard macro. Error if not at the top level in keyboard
 * processing. Set up variables and return.
 */
int ctlxlp(int f, int n)
{
  if (kbdmip != NULL || kbdmop != NULL)
    {
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
int ctlxrp(int f, int n)
{
  if (kbdmip == NULL)
    {
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
int ctlxe(int f, int n)
{
  int c, af, an, s;

  if (kbdmip != NULL || kbdmop != NULL)
    {
      mlwrite("No macro defined");
      return (FALSE);
    }
  if (n <= 0)
    return (TRUE);
  do
    {
      kbdmop = &kbdm[0];
      do
	{
	  af = FALSE;
	  an = 1;
	  if ((c = *kbdmop++) == (CTRL | 'U'))
	    {
	      af = TRUE;
	      an = *kbdmop++;
	      c = *kbdmop++;
	    }
	  s = TRUE;
	}
      while (c != (CTLX | ')') && (s = execute(c, af, an)) == TRUE);
      kbdmop = NULL;
    }
  while (s == TRUE && --n);
  return (s);
}

/*
 * Abort. Beep the beeper. Kill off any keyboard macro, etc., that is in
 * progress. Sometimes called as a routine, to do general aborting of stuff.
 */
int ctrlg(int f, int n)
{
  (*term.t_beep) ();
  if (kbdmip != NULL)
    {
      kbdm[0] = (CTLX | ')');
      kbdmip = NULL;
    }
  mlwrite("[Aborted]");
  return (ABORT);
}

/*
 * Handle ANSI escape-extended commands (with "ESC [" or "ESC O" prefix)
 */
int extendedcmd(int f, int n)
{
  int (*cmd)();
  int c;

  c = getctl();
  switch (c)
    {
    case 'A': cmd = backline; break;
    case 'B': cmd = forwline; break;
    case 'C': cmd = forwchar; break;
    case 'D': cmd = backchar; break;
    case 'H': cmd = gotobob; break;
    case 'W': cmd = gotoeob; break;
    case '5': cmd = pageup; getctl(); break;
    case '6': cmd = pagedown; getctl(); break;
    case '7': cmd = gotobob; getctl(); break;
    case '8': cmd = gotoeob; getctl(); break;
    default: mlwrite("\007[Key not bound]");
      return (FALSE);
    }
  return cmd(f, n);
}
