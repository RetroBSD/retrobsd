/* This file is in the public domain. */

/*
 * Window management. Some of the functions are internal, and some are
 * attached to keys that the user actually types
 */

#include <stdlib.h>		/* free(3), malloc(3) */
#include "estruct.h"
#include "edef.h"

extern void upmode();
extern void mlwrite();

int refresh(int f, int n);
int nextwind(int f, int n);
int prevwind(int f, int n);
int onlywind(int f, int n);
int splitwind(int f, int n);
int enlargewind(int f, int n);
int shrinkwind(int f, int n);
WINDOW* wpopup();

/*
 * Refresh the screen. With no argument, it does the refresh and centers
 * the cursor on the screen. With an argument it does a reposition instead.
 * Bound to "C-L"
 */
int refresh(int f, int n)
{
  if (n >= 0)
    n++;			/* adjust to screen row */
  if (f == FALSE)
    {
      sgarbf = TRUE;
      n = 0;			/* Center dot */
    }
  curwp->w_force = n;
  curwp->w_flag |= WFFORCE;
  return (TRUE);
}

/*
 * The command make the next window (next => down the screen) the current
 * window. There are no real errors, although the command does nothing if
 * there is only 1 window on the screen. Bound to "C-X C-N"
 */
int nextwind(int f, int n)
{
  WINDOW *wp;

  if ((wp = curwp->w_wndp) == NULL)
    wp = wheadp;

  curwp = wp;
  curbp = wp->w_bufp;
  upmode();
  return (TRUE);
}

/*
 * This command makes the previous window (previous => up the screen) the
 * current window. There arn't any errors, although the command does not do a
 * lot if there is 1 window
 */
int prevwind(int f, int n)
{
  WINDOW *wp1, *wp2;

  wp1 = wheadp;
  wp2 = curwp;

  if (wp1 == wp2)
    wp2 = NULL;

  while (wp1->w_wndp != wp2)
    wp1 = wp1->w_wndp;

  curwp = wp1;
  curbp = wp1->w_bufp;
  upmode();
  return (TRUE);
}

/*
 * This command makes the current window the only window on the screen. Bound
 * to "C-X 1". Try to set the framing so that "." does not have to move on the
 * display. Some care has to be taken to keep the values of dot and mark in
 * the buffer structures right if the distruction of a window makes a buffer
 * become undisplayed
 */
int onlywind(int f, int n)
{
  WINDOW *wp;
  LINE *lp;
  int i;

  while (wheadp != curwp)
    {
      wp = wheadp;
      wheadp = wp->w_wndp;
      if (--wp->w_bufp->b_nwnd == 0)
	{
	  wp->w_bufp->b_dotp = wp->w_dotp;
	  wp->w_bufp->b_doto = wp->w_doto;
	  wp->w_bufp->b_markp = wp->w_markp;
	  wp->w_bufp->b_marko = wp->w_marko;
	}
      free((char *) wp);
    }
  while (curwp->w_wndp != NULL)
    {
      wp = curwp->w_wndp;
      curwp->w_wndp = wp->w_wndp;
      if (--wp->w_bufp->b_nwnd == 0)
	{
	  wp->w_bufp->b_dotp = wp->w_dotp;
	  wp->w_bufp->b_doto = wp->w_doto;
	  wp->w_bufp->b_markp = wp->w_markp;
	  wp->w_bufp->b_marko = wp->w_marko;
	}
      free((char *) wp);
    }
  lp = curwp->w_linep;
  i = curwp->w_toprow;
  while (i != 0 && lback (lp) != curbp->b_linep)
    {
      --i;
      lp = lback(lp);
    }
  curwp->w_toprow = 0;
  curwp->w_ntrows = term.t_nrow - 1;
  curwp->w_linep = lp;
  curwp->w_flag |= WFMODE | WFHARD;
  return (TRUE);
}

/*
 * Split the current window. A window smaller than 3 lines cannot be split.
 * The only other error that is possible is a "malloc" failure allocating the
 * structure for the new window. Bound to "C-X 2"
 */
int splitwind(int f, int n)
{
  LINE *lp;
  WINDOW *wp, *wp1, *wp2;
  int ntru, ntrl, ntrd;

  if (curwp->w_ntrows < 3)
    {
      mlwrite("Cannot split a %d line window", curwp->w_ntrows);
      return (FALSE);
    }
  if ((wp = (WINDOW *) malloc(sizeof(WINDOW))) == NULL)
    {
      mlwrite("Cannot allocate WINDOW block");
      return (FALSE);
    }
  ++curbp->b_nwnd;		/* Displayed twice */
  wp->w_bufp = curbp;
  wp->w_dotp = curwp->w_dotp;
  wp->w_doto = curwp->w_doto;
  wp->w_markp = curwp->w_markp;
  wp->w_marko = curwp->w_marko;
  wp->w_flag = 0;
  wp->w_force = 0;
  ntru = (curwp->w_ntrows - 1) / 2; /* Upper size */
  ntrl = (curwp->w_ntrows - 1) - ntru; /* Lower size */
  lp = curwp->w_linep;
  ntrd = 0;
  while (lp != curwp->w_dotp)
    {
      ++ntrd;
      lp = lforw(lp);
    }
  lp = curwp->w_linep;
  if (ntrd <= ntru)
    {				/* Old is upper window */
      if (ntrd == ntru)		/* Hit mode line */
	lp = lforw(lp);
      curwp->w_ntrows = ntru;
      wp->w_wndp = curwp->w_wndp;
      curwp->w_wndp = wp;
      wp->w_toprow = curwp->w_toprow + ntru + 1;
      wp->w_ntrows = ntrl;
    }
  else
    {				/* Old is lower window */
      wp1 = NULL;
      wp2 = wheadp;
      while (wp2 != curwp)
	{
	  wp1 = wp2;
	  wp2 = wp2->w_wndp;
	}
      if (wp1 == NULL)
	wheadp = wp;
      else
	wp1->w_wndp = wp;
      wp->w_wndp = curwp;
      wp->w_toprow = curwp->w_toprow;
      wp->w_ntrows = ntru;
      ++ntru;			/* Mode line */
      curwp->w_toprow += ntru;
      curwp->w_ntrows = ntrl;
      while (ntru--)
	lp = lforw (lp);
    }
  curwp->w_linep = lp;		/* Adjust the top lines */
  wp->w_linep = lp;		/* if necessary */
  curwp->w_flag |= WFMODE | WFHARD;
  wp->w_flag |= WFMODE | WFHARD;
  return (TRUE);
}

/*
 * Enlarge the current window. Find the window that loses space. Make sure it
 * is big enough. If so, hack the window descriptions, and ask redisplay to do
 * all the hard work. You don't just set "force reframe" because dot would
 * move. Bound to "C-X Z"
 */
int enlargewind(int f, int n)
{
  WINDOW *adjwp;
  LINE *lp;
  int i;

  if (n < 0)
    return (shrinkwind(f, -n));
  if (wheadp->w_wndp == NULL)
    {
      mlwrite("Only one window");
      return (FALSE);
    }
  if ((adjwp = curwp->w_wndp) == NULL)
    {
      adjwp = wheadp;
      while (adjwp->w_wndp != curwp)
	adjwp = adjwp->w_wndp;
    }
  if (adjwp->w_ntrows <= n)
    {
      mlwrite("Impossible change");
      return (FALSE);
    }
  if (curwp->w_wndp == adjwp)
    {				/* Shrink below */
      lp = adjwp->w_linep;
      for (i = 0; i < n && lp != adjwp->w_bufp->b_linep; ++i)
	lp = lforw(lp);
      adjwp->w_linep = lp;
      adjwp->w_toprow += n;
    }
  else
    {				/* Shrink above */
      lp = curwp->w_linep;
      for (i = 0; i < n && lback(lp) != curbp->b_linep; ++i)
	lp = lback(lp);
      curwp->w_linep = lp;
      curwp->w_toprow -= n;
    }
  curwp->w_ntrows += n;
  adjwp->w_ntrows -= n;
  curwp->w_flag |= WFMODE | WFHARD;
  adjwp->w_flag |= WFMODE | WFHARD;
  return (TRUE);
}

/*
 * Shrink the current window. Find the window that gains space. Hack at the
 * window descriptions. Ask the redisplay to do all the hard work
 */
int shrinkwind(int f, int n)
{
  WINDOW *adjwp;
  LINE *lp;
  int i;

  if (n < 0)
    return (enlargewind(f, -n));
  if (wheadp->w_wndp == NULL)
    {
      mlwrite("Only one window");
      return (FALSE);
    }
  if ((adjwp = curwp->w_wndp) == NULL)
    {
      adjwp = wheadp;
      while (adjwp->w_wndp != curwp)
	adjwp = adjwp->w_wndp;
    }
  if (curwp->w_ntrows <= n)
    {
      mlwrite("Impossible change");
      return (FALSE);
    }
  if (curwp->w_wndp == adjwp)
    {				/* Grow below */
      lp = adjwp->w_linep;
      for (i = 0; i < n && lback(lp) != adjwp->w_bufp->b_linep; ++i)
	lp = lback(lp);
      adjwp->w_linep = lp;
      adjwp->w_toprow -= n;
    }
  else
    {				/* Grow above */
      lp = curwp->w_linep;
      for (i = 0; i < n && lp != curbp->b_linep; ++i)
	lp = lforw(lp);
      curwp->w_linep = lp;
      curwp->w_toprow += n;
    }
  curwp->w_ntrows -= n;
  adjwp->w_ntrows += n;
  curwp->w_flag |= WFMODE | WFHARD;
  adjwp->w_flag |= WFMODE | WFHARD;
  return (TRUE);
}

/*
 * Pick a window for a pop-up. Split the screen if there is only one window.
 * Pick the uppermost window that isn't the current window. An LRU algorithm
 * might be better. Return a pointer, or NULL on error
 */
WINDOW* wpopup()
{
  WINDOW *wp;

  if (wheadp->w_wndp == NULL	/* Only 1 window */
      && splitwind(FALSE, 0) == FALSE)	/* and it won't split */
    return (NULL);
  wp = wheadp;			/* Find window to use */
  while (wp != NULL && wp == curwp)
    wp = wp->w_wndp;
  return (wp);
}
