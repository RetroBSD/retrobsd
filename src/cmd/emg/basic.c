/* This file is in the public domain. */

/*
 * The routines in this file move the cursor around on the screen. They
 * compute a new value for the cursor, then adjust ".". The display code
 * always updates the cursor location, so only moves between lines, or
 * functions that adjust the top line in the window and invalidate the
 * framing, are hard.
 */

#include <stdlib.h>		/* atoi(3), ugh */
#include "estruct.h"
#include "edef.h"

extern int getccol(int);
extern void mlwrite();
extern int mlreplyt();

int forwchar(int, int);
int backchar(int, int);
int forwline(int, int);
int backline(int, int);
int pagedown(int, int);
int pageup(int, int);

/*
 * This routine, given a pointer to a LINE, and the current cursor goal
 * column, return the best choice for the offset. The offset is returned.
 * Used by "C-N" and "C-P".
 */
long getgoal(LINE *dlp)
{
  int col = 0;
  int dbo = 0;
  int newcol, c;

  while (dbo != llength(dlp))
    {
      c = lgetc(dlp, dbo);
      newcol = col;
      if (c == '\t')
	newcol |= 0x07;
      else if (c < 0x20 || c == 0x7F)
	++newcol;
      ++newcol;
      if (newcol > curgoal)
	break;
      col = newcol;
      ++dbo;
    }
  return (dbo);
}

/*
 * Move the cursor to the
 * beginning of the current line.
 * Trivial.
 */
/* ARGSUSED0 */
int gotobol(int f, int n)
{
  curwp->w_doto = 0;
  return (TRUE);
}

/*
 * Move the cursor to the end of the current line. Trivial. No errors.
 */
/* ARGSUSED0 */
int gotoeol(int f, int n)
{
  curwp->w_doto = llength(curwp->w_dotp);
  return (TRUE);
}

/*
 * Move the cursor backwards by "n" characters. If "n" is less than zero call
 * "forwchar" to actually do the move. Otherwise compute the new cursor
 * location. Error if you try and move out of the buffer. Set the flag if the
 * line pointer for dot changes.
 */
int backchar(int f, int n)
{
  LINE *lp;

  if (n < 0)
    return (forwchar(f, -n));
  while (n--)
    {
      if (curwp->w_doto == 0)
	{
	  if ((lp = lback(curwp->w_dotp)) == curbp->b_linep)
	    return (FALSE);
	  curwp->w_dotp = lp;
	  curwp->w_doto = llength(lp);
	  curwp->w_flag |= WFMOVE;
	  curwp->w_dotline--;
	}
      else
	curwp->w_doto--;
    }
  return (TRUE);
}

/*
 * Move the cursor forwards by "n" characters. If "n" is less than zero call
 * "backchar" to actually do the move. Otherwise compute the new cursor
 * location, and move ".". Error if you try and move off the end of the
 * buffer. Set the flag if the line pointer for dot changes.
 */
int forwchar(int f, int n)
{
  if (n < 0)
    return (backchar(f, -n));
  while (n--)
    {
      if (curwp->w_doto == llength(curwp->w_dotp))
	{
	  if (curwp->w_dotp == curbp->b_linep)
	    return (FALSE);
	  curwp->w_dotp = lforw(curwp->w_dotp);
	  curwp->w_doto = 0;
	  curwp->w_flag |= WFMOVE;
	  curwp->w_dotline++;
	}
      else
	curwp->w_doto++;
    }
  return (TRUE);
}

/*
 * move to a particular line. argument (n) must be a positive integer for this
 * to actually do anything
 */
int gotoline(int f, int n)
{
  if ((n < 1) || (n > curwp->w_bufp->b_lines))	/* if a bogus argument...then leave */
    return (FALSE);				/* but we should never get here */

  /* first, we go to the start of the buffer */
  curwp->w_dotp = lforw(curbp->b_linep);
  curwp->w_doto = 0;
  curwp->w_dotline = 0;		/* and reset the line number */
  return (forwline(f, n - 1));
}

/*
 * Prompt for which line number we want to go to, then execute gotoline()
 * with that number as its argument.
 *
 * Make sure the bounds are within the file.
 *
 * Bound to M-G
 */
int setline(int f, int n)
{
  char setl[6];
  int l;

  (void)mlreplyt("Go to line: ", setl, 6, 10);
  l = atoi(setl);		/* XXX: This sucks! */

  if (l < 1)
    l = 1;
  else if (l > curwp->w_bufp->b_lines)
    l = curwp->w_bufp->b_lines;

  gotoline(f, l);
  return (TRUE);
}

/*
 * Goto the beginning of the buffer. Massive adjustment of dot. This is
 * considered to be hard motion; it really isn't if the original value of dot
 * is the same as the new value of dot. Normally bound to "M-<".
 */
/* ARGSUSED0 */
int gotobob(int f, int n)
{
  curwp->w_dotp = lforw(curbp->b_linep);
  curwp->w_doto = 0;
  curwp->w_flag |= WFHARD;
  curwp->w_dotline = 0;
  return (TRUE);
}

/*
 * Move to the end of the buffer. Dot is always put at the end of the file
 * (ZJ). The standard screen code does most of the hard parts of update.
 * Bound to "M->".
 */
/* ARGSUSED0 */
int gotoeob(int f, int n)
{
  curwp->w_dotp = curbp->b_linep;
  curwp->w_doto = 0;
  curwp->w_flag |= WFHARD;
  curwp->w_dotline = curwp->w_bufp->b_lines;
  return (TRUE);
}

/*
 * Move forward by full lines. If the number of lines to move is less than
 * zero, call the backward line function to actually do it. The last command
 * controls how the goal column is set. Bound to "C-N". No errors are possible.
 */
int forwline(int f, int n)
{
  LINE *dlp;

  if (n < 0)
    return (backline(f, -n));
  if ((lastflag & CFCPCN) == 0)/* Reset goal if last */
    curgoal = getccol(FALSE); /* not C-P or C-N */
  thisflag |= CFCPCN;
  dlp = curwp->w_dotp;
  while (n-- && dlp != curbp->b_linep)
  {
    dlp = lforw(dlp);
    curwp->w_dotline++;
  }
  curwp->w_dotp = dlp;
  curwp->w_doto = getgoal(dlp);
  curwp->w_flag |= WFMOVE;
  return (TRUE);
}

/*
 * This function is like "forwline", but goes backwards. The scheme is exactly
 * the same. Check for arguments that are less than zero and call your
 * alternate. Figure out the new line and call "movedot" to perform the
 * motion. No errors are possible. Bound to "C-P".
 */
int backline(int f, int n)
{
  LINE *dlp;

  if (n < 0)
    return (forwline(f, -n));
  if ((lastflag & CFCPCN) == 0)/* Reset goal if the */
    curgoal = getccol(FALSE); /* last isn't C-P, C-N */
  thisflag |= CFCPCN;
  dlp = curwp->w_dotp;
  while (n-- && lback(dlp) != curbp->b_linep)
  {
    dlp = lback(dlp);
    curwp->w_dotline--;
  }
  curwp->w_dotp = dlp;
  curwp->w_doto = getgoal(dlp);
  curwp->w_flag |= WFMOVE;
  return (TRUE);
}

/*
 * PgDn. Scroll down (rows / 2).
 * Just forwline(f, (rows / 2))
 * Bound to C-V
 */
int pagedown(int f, int n)
{
  forwline(f, (rows / 2));
  return (TRUE);
}

/*
 * PgUp. Scroll up (rows / 2).
 * Just backline(f, (rows / 2))
 * Bound to M-V
 */
int pageup(int f, int n)
{
  backline(f, (rows / 2));
  return (TRUE);
}

/*
 * Set the mark in the current window to the value of "." in the window. No
 * errors are possible. Bound to "M-.".
 */
/* ARGSUSED0 */
int setmark(int f, int n)
{
  curwp->w_markp = curwp->w_dotp;
  curwp->w_marko = curwp->w_doto;
  mlwrite("[Mark set]");
  return (TRUE);
}
