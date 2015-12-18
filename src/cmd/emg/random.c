/* This file is in the public domain. */

/*
 * This file contains the command processing functions for a number of random
 * commands. There is no functional grouping here, for sure.
 */

#include "estruct.h"
#include "edef.h"

extern void mlwrite();
extern void lchange(int);
extern int lnewline();
extern int linsert(int, int);
extern int backchar(int, int);
extern void kdelete();
extern int ldelete(int, int);
extern int kremove(int);

int setfillcol(int, int);
int getccol(int);
int twiddle(int, int);
int quote(int, int);
int tab(int, int);
int openline(int, int);
int newline(int, int);
int forwdel(int, int);
int backdel(int, int);
int killtext(int, int);
int yank(int, int);

/*
 * Set fill column to n.
 */
int setfillcol(int f, int n)
{
  fillcol = n;
  mlwrite("[Fill column is %d]", n);
  return (TRUE);
}

/*
 * Return current column.  Stop at first non-blank given TRUE argument.
 */
int getccol(int bflg)
{
  int c, i, col;

  col = 0;
  for (i = 0; i < curwp->w_doto; ++i)
    {
      c = lgetc(curwp->w_dotp, i);
      if (c != ' ' && c != '\t' && bflg)
	break;
      if (c == '\t')
	col |= 0x07;
      else if (c < 0x20 || c == 0x7F)
	++col;
      ++col;
    }
  return (col);
}

/*
 * Twiddle the two characters on either side of dot. If dot is at the end of
 * the line twiddle the two characters before it. Return with an error if dot
 * is at the beginning of line; it seems to be a bit pointless to make this
 * work. This fixes up a very common typo with a single stroke. Normally bound
 * to "C-T". This always works within a line, so "WFEDIT" is good enough
 */
int twiddle(int f, int n)
{
  LINE *dotp;
  int doto, cl, cr;

  dotp = curwp->w_dotp;
  doto = curwp->w_doto;
  if (doto == llength(dotp) && --doto < 0)
    return (FALSE);
  cr = lgetc(dotp, doto);
  if (--doto < 0)
    return (FALSE);
  cl = lgetc(dotp, doto);
  lputc(dotp, doto + 0, cr);
  lputc(dotp, doto + 1, cl);
  lchange(WFEDIT);
  return (TRUE);
}

/*
 * Quote the next character, and insert it into the buffer. All the characters
 * are taken literally, with the exception of the newline, which always has
 * its line splitting meaning. The character is always read, even if it is
 * inserted 0 times, for regularity. Bound to "C-Q"
 */
int quote(int f, int n)
{
  int s, c;

  c = (*term.t_getchar) ();
  if (n < 0)
    return (FALSE);
  if (n == 0)
    return (TRUE);
  if (c == '\n')
    {
      do
	{
	  s = lnewline();
	}
      while (s == TRUE && --n);
      return (s);
    }
  return (linsert(n, c));
}

/*
 * Insert a tab into file.
 * Bound to "C-I"
 */
int tab(int f, int n)
{
  if (n < 0)
    return (FALSE);
  return (linsert(n, 9));
}

/*
 * Open up some blank space. The basic plan is to insert a bunch of newlines,
 * and then back up over them. Everything is done by the subcommand
 * processors. They even handle the looping. Normally this is bound to "C-O"
 */
int openline(int f, int n)
{
  int i, s;

  if (n < 0)
    return (FALSE);
  if (n == 0)
    return (TRUE);
  i = n;			/* Insert newlines */
  do
    {
      s = lnewline();
    }
  while (s == TRUE && --i);
  if (s == TRUE)		/* Then back up overtop */
    s = backchar(f, n);		/* of them all */
  return (s);
}

/*
 * Insert a newline. Bound to "C-M".
 */
int newline(int f, int n)
{
  int s;

  if (n < 0)
    return (FALSE);

  /* insert some lines */
  while (n--)
    {
      if ((s = lnewline()) != TRUE)
	return (s);
    }
  return (TRUE);
}

/*
 * Delete forward. This is real easy, because the basic delete routine does
 * all of the work. Watches for negative arguments, and does the right thing.
 * If any argument is present, it kills rather than deletes, to prevent loss
 * of text if typed with a big argument. Normally bound to "C-D"
 */
int forwdel(int f, int n)
{
  if (n < 0)
    return (backdel(f, -n));
  if (f != FALSE)
    {			       /* Really a kill */
      if ((lastflag & CFKILL) == 0)
	kdelete();
      thisflag |= CFKILL;
    }
  return (ldelete(n, f));
}

/*
 * Delete backwards. This is quite easy too, because it's all done with other
 * functions. Just move the cursor back, and delete forwards. Like delete
 * forward, this actually does a kill if presented with an argument. Bound to
 * both "RUBOUT" and "C-H"
 */
int backdel(int f, int n)
{
  int s;

  if (n < 0)
    return (forwdel(f, -n));
  if (f != FALSE)
    {			       /* Really a kill */
      if ((lastflag & CFKILL) == 0)
	kdelete();
      thisflag |= CFKILL;
    }
  if ((s = backchar(f, n)) == TRUE)
    s = ldelete(n, f);
  return (s);
}

/*
 * Kill text. If called without an argument, it kills from dot to the end of
 * the line, unless it is at the end of the line, when it kills the newline.
 * If called with an argument of 0, it kills from the start of the line to
 * dot. If called with a positive argument, it kills from dot forward over
 * that number of newlines. If called with a negative argument it kills
 * backwards that number of newlines. Normally bound to "C-K"
 */
int killtext(int f, int n)
{
  LINE *nextp;
  int chunk;

  if ((lastflag & CFKILL) == 0)/* Clear kill buffer if last wasn't a kill */
    kdelete();
  thisflag |= CFKILL;
  if (f == FALSE)
    {
      chunk = llength(curwp->w_dotp) - curwp->w_doto;
      if (chunk == 0)
	chunk = 1;
    }
  else if (n == 0)
    {
      chunk = curwp->w_doto;
      curwp->w_doto = 0;
    }
  else if (n > 0)
    {
      chunk = llength(curwp->w_dotp) - curwp->w_doto + 1;
      nextp = lforw(curwp->w_dotp);
      while (--n)
	{
	  if (nextp == curbp->b_linep)
	    return (FALSE);
	  chunk += llength(nextp) + 1;
	  nextp = lforw(nextp);
	}
    }
  else
    {
      mlwrite("neg kill");
      return (FALSE);
    }
  return (ldelete(chunk, TRUE));
}

/*
 * Yank text back from the kill buffer. This is really easy. All of the work
 * is done by the standard insert routines. All you do is run the loop, and
 * check for errors. Bound to "C-Y"
 */
int yank(int f, int n)
{
  int c, i;

  if (n < 0)
    return (FALSE);
  while (n--)
    {
      i = 0;
      while ((c = kremove(i)) >= 0)
	{
	  if (c == '\n')
	    {
	      if (lnewline(FALSE, 1) == FALSE)
		return (FALSE);
	    }
	  else
	    {
	      if (linsert(1, c) == FALSE)
		return (FALSE);
	    }
	  ++i;
	}
    }
  return (TRUE);
}
