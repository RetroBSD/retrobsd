/* This file is in the public domain. */

/*
 * The functions in this file handle redisplay. There are two halves, the ones
 * that update the virtual display screen, and the ones that make the physical
 * display screen the same as the virtual display screen. These functions use
 * hints that are left in the windows by the commands
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "estruct.h"
#include "edef.h"

extern int typeahead();
extern int ctrlg();
extern int getccol();

void movecursor(int, int);
void mlerase();
int  refresh();
void vtinit();
void vttidy();
void vtmove(int, int);
void vtputc(int);
void vtpute(int);
int  vtputs(const char *);
void vteeol();
void update();
void updext();
void updateline(int, char [], char [], short *);
void modeline(WINDOW *);
void upmode();
int mlyesno(char *);
int mlreplyt(char *, char *, int, char);
int mlreply(char *, char *, int);
void mlwrite();
void mlputs(char *);
void mlputi(int, int);
void mlputli(long, int);

typedef struct VIDEO {
  short v_flag;			/* Flags */
  char v_text[1];		/* Screen data */
} VIDEO;

#define VFCHG	0x0001		/* Changed flag */
#define VFEXT	0x0002		/* extended (beyond column 80) */
#define VFREV	0x0004		/* reverse video status */
#define VFREQ	0x0008		/* reverse video request */

int vtrow = 0;			/* Row location of SW cursor */
int vtcol = 0;			/* Column location of SW cursor */
int ttrow = HUGE;		/* Row location of HW cursor */
int ttcol = HUGE;		/* Column location of HW cursor */
int lbound = 0;			/* leftmost column of line being displayed */

VIDEO **vscreen;		/* Virtual screen */
VIDEO **pscreen;		/* Physical screen */

/*
 * Refresh the screen. With no argument, it does the refresh and centers
 * the cursor on the screen. With an argument it does a reposition instead.
 * Bound to "C-L:
 */
int
refresh(int f, int n)
{
  if (n >= 0)
    n++;			/* adjust to screen row */
  if (f == FALSE) {
    sgarbf = TRUE;
    n = 0;			/* Center dot */
  }
  curwp->w_force = n;
  curwp->w_flag |= WFFORCE;
  return (TRUE);
}

/*
 * Send a command to the terminal to move the hardware cursor to row "row" and
 * column "col". The row and column arguments are origin 0. Optimize out
 * random calls. Update "ttrow" and "ttcol".
 */
void movecursor(int row, int col)
{
  if (row != ttrow || col != ttcol)
    {
      ttrow = row;
      ttcol = col;
      (*term.t_move) (row, col);
    }
}

/*
 * Erase the message line. This is a special routine because the message line
 * is not considered to be part of the virtual screen. It always works
 * immediately; the terminal buffer is flushed via a call to the flusher.
 */
void mlerase()
{
  int i;

  movecursor(term.t_nrow, 0);
  if (eolexist == TRUE)
    (*term.t_eeol) ();
  else
    {
      for (i = 0; i < term.t_ncol - 1; i++)
	(*term.t_putchar) (' ');
      movecursor(term.t_nrow, 1);	  /* force the move! */
      movecursor(term.t_nrow, 0);
    }
  (*term.t_flush) ();
  mpresf = FALSE;
}

/*
 * Initialize the data structures used by the display code. The edge vectors
 * used to access the screens are set up. The operating system's terminal I/O
 * channel is set up. All the other things get initialized at compile time.
 * The original window has "WFCHG" set, so that it will get completely redrawn
 * on the first call to "update".
 */
void vtinit()
{
  int i;
  VIDEO *vp;

  (*term.t_open) ();
  (*term.t_rev) (FALSE);
  vscreen = (VIDEO **) malloc(term.t_nrow * sizeof(VIDEO *));

  if (vscreen == NULL)
    exit (1);

  pscreen = (VIDEO **) malloc(term.t_nrow * sizeof(VIDEO *));

  if (pscreen == NULL)
    exit (1);

  for (i = 0; i < term.t_nrow; ++i)
    {
      vp = (VIDEO *) malloc(sizeof(VIDEO) + term.t_ncol);

      if (vp == NULL)
	exit (1);

      vp->v_flag = 0;
      vscreen[i] = vp;
      vp = (VIDEO *) malloc(sizeof(VIDEO) + term.t_ncol);

      if (vp == NULL)
	exit (1);

      vp->v_flag = 0;
      pscreen[i] = vp;
    }
}

/*
 * Clean up the virtual terminal system, in anticipation for a return to the
 * operating system. Move down to the last line and clear it out (the next
 * system prompt will be written in the line). Shut down the channel to the
 * terminal.
 */
void vttidy()
{
  mlerase();
  movecursor(term.t_nrow, 0);
  (*term.t_close) ();
}

/*
 * Set the virtual cursor to the specified row and column on the virtual
 * screen. There is no checking for nonsense values; this might be a good idea
 * during the early stages.
 */
void vtmove(int row, int col)
{
  vtrow = row;
  vtcol = col;
}

/*
 * Write a character to the virtual screen. The virtual row and column are
 * updated. If the line is too long put a "$" in the last column. This routine
 * only puts printing characters into the virtual terminal buffers. Only
 * column overflow is checked.
 */
void vtputc(int c)
{
  VIDEO *vp;

  vp = vscreen[vtrow];

  if (vtcol >= term.t_ncol)
    {
      vtcol = (vtcol + 0x07) & ~0x07;
      vp->v_text[term.t_ncol - 1] = '$';
    }
  else if (c == '\t')
    {
      do
	{
	  vtputc(' ');
	}
      while ((vtcol & 0x07) != 0);
    }
  else if (c < 0x20 || c == 0x7F)
    {
      vtputc('^');
      vtputc(c ^ 0x40);
    }
  else
    vp->v_text[vtcol++] = c;
}

/* put a character to the virtual screen in an extended line. If we are not
 * yet on left edge, don't print it yet. check for overflow on the right
 * margin
 */
void vtpute(int c)
{
  VIDEO *vp;

  vp = vscreen[vtrow];

  if (vtcol >= term.t_ncol)
    {
      vtcol = (vtcol + 0x07) & ~0x07;
      vp->v_text[term.t_ncol - 1] = '$';
    }
  else if (c == '\t')
    {
      do
	{
	  vtpute(' ');
	}
      while (((vtcol + lbound) & 0x07) != 0);
    }
  else if (c < 0x20 || c == 0x7F)
    {
      vtpute('^');
      vtpute(c ^ 0x40);
    }
  else
    {
      if (vtcol >= 0)
	vp->v_text[vtcol] = c;
      ++vtcol;
    }
}

/*
 * Output a string to the mode line, report how long it was.
 * From OpenBSD mg.
 */
int vtputs(const char *s)
{
  int n = 0;

  while (*s != '\0') {
    vtputc(*s++);
    ++n;
  }
  return (n);
}

/*
 * [In the virtual screen] Erase from the end of the software cursor to the
 * end of the line on which the software cursor is located.
 */
void vteeol()
{
  VIDEO *vp;

  vp = vscreen[vtrow];
  while (vtcol < term.t_ncol)
    vp->v_text[vtcol++] = ' ';
}

/*
 * Make sure that the display is right. This is a three part process. First,
 * scan through all of the windows looking for dirty ones. Check the framing,
 * and refresh the screen. Second, make sure that "currow" and "curcol" are
 * correct for the current window. Third, make the virtual and physical
 * screens the same
 */
void update()
{
  VIDEO *vp1, *vp2;
  LINE *lp;
  WINDOW *wp;
  int i, j, c;

  if (typeahead())
    return;

  /* update the reverse video flags for any mode lines out there */
  for (i = 0; i < term.t_nrow; ++i)
    vscreen[i]->v_flag &= ~VFREQ;

  wp = wheadp;
  while (wp != NULL)
    {
      vscreen[wp->w_toprow + wp->w_ntrows]->v_flag |= VFREQ;
      wp = wp->w_wndp;
    }

  wp = wheadp;
  while (wp != NULL)
    {
      /* Look at any window with update flags set on */
      if (wp->w_flag != 0)
	{
	  /* If not force reframe, check the framing */
	  if ((wp->w_flag & WFFORCE) == 0)
	    {
	      lp = wp->w_linep;
	      for (i = 0; i < wp->w_ntrows; ++i)
		{
		  if (lp == wp->w_dotp)
		    goto out;
		  if (lp == wp->w_bufp->b_linep)
		    break;
		  lp = lforw(lp);
		}
	    }
	  /* Not acceptable, better compute a new value for the line at the top
	   * of the window. Then set the "WFHARD" flag to force full redraw */
	  i = wp->w_force;
	  if (i > 0)
	    {
	      --i;
	      if (i >= wp->w_ntrows)
		i = wp->w_ntrows - 1;
	    }
	  else if (i < 0)
	    {
	      i += wp->w_ntrows;
	      if (i < 0)
		i = 0;
	    }
	  else
	    i = wp->w_ntrows / 2;
	  lp = wp->w_dotp;
	  while (i != 0 && lback(lp) != wp->w_bufp->b_linep)
	    {
	      --i;
	      lp = lback(lp);
	    }
	  wp->w_linep = lp;
	  wp->w_flag |= WFHARD;	   /* Force full */

	out:
	  /* Try to use reduced update. Mode line update has its own special
	   * flag. The fast update is used if the only thing to do is within
	   * the line editing */
	  lp = wp->w_linep;
	  i = wp->w_toprow;
	  if ((wp->w_flag & ~WFMODE) == WFEDIT)
	    {
	      while (lp != wp->w_dotp)
		{
		  ++i;
		  lp = lforw(lp);
		}
	      vscreen[i]->v_flag |= VFCHG;
	      vtmove(i, 0);
	      for (j = 0; j < llength(lp); ++j)
		vtputc(lgetc(lp, j));
	      vteeol();
	    }
	  else if ((wp->w_flag & (WFEDIT | WFHARD)) != 0)
	    {
	      while (i < wp->w_toprow + wp->w_ntrows)
		{
		  vscreen[i]->v_flag |= VFCHG;
		  vtmove(i, 0);
		  /* if line has been changed */
		  if (lp != wp->w_bufp->b_linep)
		    {
		      for (j = 0; j < llength(lp); ++j)
			vtputc(lgetc(lp, j));
		      lp = lforw(lp);
		    }
		  vteeol();
		  ++i;
		}
	    }
	}
      modeline(wp);		/* always update the modeline so line number is correct */
      wp->w_flag = 0;
      wp->w_force = 0;

      wp = wp->w_wndp;		/* and onward to the next window */
    }

  /* Always recompute the row and column number of the hardware cursor. This
   * is the only update for simple moves */
  lp = curwp->w_linep;
  currow = curwp->w_toprow;
  while (lp != curwp->w_dotp)
    {
      ++currow;
      lp = lforw(lp);
    }

  curcol = 0;
  i = 0;
  while (i < curwp->w_doto)
    {
      c = lgetc(lp, i++);
      if (c == '\t')
	curcol |= 0x07;
      else if (c < 0x20 || c == 0x7F)
	++curcol;
      ++curcol;
    }
  if (curcol >= term.t_ncol - 1)
    {				 /* extended line */
      /* flag we are extended and changed */
      vscreen[currow]->v_flag |= VFEXT | VFCHG;
      updext();			/* and output extended line */
    }
  else
    lbound = 0;			/* not extended line */

  /* make sure no lines need to be de-extended because the cursor is no
   * longer on them */

  wp = wheadp;

  while (wp != NULL)
    {
      lp = wp->w_linep;
      i = wp->w_toprow;

      while (i < wp->w_toprow + wp->w_ntrows)
	{
	  if (vscreen[i]->v_flag & VFEXT)
	    {
	      /* always flag extended lines as changed */
	      vscreen[i]->v_flag |= VFCHG;
	      if ((wp != curwp) || (lp != wp->w_dotp) ||
		  (curcol < term.t_ncol - 1))
		{
		  vtmove(i, 0);
		  for (j = 0; j < llength(lp); ++j)
		    vtputc (lgetc(lp, j));
		  vteeol();
		  /* this line no longer is extended */
		  vscreen[i]->v_flag &= ~VFEXT;
		}
	    }
	  lp = lforw(lp);
	  ++i;
	}
      /* and onward to the next window */
      wp = wp->w_wndp;
    }

  /* Special hacking if the screen is garbage. Clear the hardware screen, and
   * update your copy to agree with it. Set all the virtual screen change
   * bits, to force a full update */
  if (sgarbf != FALSE)
    {
      for (i = 0; i < term.t_nrow; ++i)
	{
	  vscreen[i]->v_flag |= VFCHG;
	  vp1 = pscreen[i];
	  for (j = 0; j < term.t_ncol; ++j)
	    vp1->v_text[j] = ' ';
	}

      movecursor(0, 0);	 /* Erase the screen */
      (*term.t_eeop) ();
      sgarbf = FALSE;	 /* Erase-page clears */
      mpresf = FALSE;	 /* the message area */
    }
  /* Make sure that the physical and virtual displays agree. Unlike before,
   * the "updateline" code is only called with a line that has been updated
   * for sure */
  for (i = 0; i < term.t_nrow; ++i)
    {
      vp1 = vscreen[i];

      /* for each line that needs to be updated, or that needs its reverse
       * video status changed, call the line updater */
      j = vp1->v_flag;
      if (((j & VFCHG) != 0) || (((j & VFREV) == 0) != ((j & VFREQ) == 0)))
	{
	  if (typeahead())
	    return;
	  vp2 = pscreen[i];
	  updateline(i, &vp1->v_text[0], &vp2->v_text[0], &vp1->v_flag);
	}
    }

  /* Finally, update the hardware cursor and flush out buffers */
  movecursor(currow, curcol - lbound);
  (*term.t_flush) ();
}

/* updext: update the extended line which the cursor is currently on at a
 * column greater than the terminal width. The line will be scrolled right or
 * left to let the user see where the cursor is
 */
void updext()
{
  LINE *lp;			/* pointer to current line */
  int rcursor;			/* real cursor location */
  int j;			/* index into line */

  /* calculate what column the real cursor will end up in */
  rcursor = ((curcol - term.t_ncol) % term.t_scrsiz) + term.t_margin;
  lbound = curcol - rcursor + 1;

  /* scan through the line outputing characters to the virtual screen */
  /* once we reach the left edge */
  vtmove(currow, -lbound);	/* start scanning offscreen */
  lp = curwp->w_dotp;		/* line to output */
  for (j = 0; j < llength(lp); ++j) /* until the end-of-line */
    vtpute (lgetc(lp, j));
  /* truncate the virtual line */
  vteeol();
  /* and put a '$' in column 1 */
  vscreen[currow]->v_text[0] = '$';
}

/*
 * Update a single line. This does not know how to use insert or delete
 * character sequences; we are using VT52 functionality. Update the physical
 * row and column variables. It does try an exploit erase to end of line.
 */
void updateline(int row, char vline[], char pline[], short *flags)
{
  char *cp1, *cp2, *cp3, *cp4, *cp5;
  int nbflag;			/* non-blanks to the right flag? */
  int rev;			/* reverse video flag */
  int req;			/* reverse video request flag */

  /* set up pointers to virtual and physical lines */
  cp1 = &vline[0];
  cp2 = &pline[0];

  /* if we need to change the reverse video status of the current line, we
   * need to re-write the entire line */
  rev = *flags & VFREV;
  req = *flags & VFREQ;
  if (rev != req)
    {
      movecursor(row, 0);	 /* Go to start of line */
      (*term.t_rev) (req != FALSE);	  /* set rev video if needed */

      /* scan through the line and dump it to the screen and the virtual
       * screen array */
      cp3 = &vline[term.t_ncol];
      while (cp1 < cp3)
	{
	  (*term.t_putchar) (*cp1);
	  ++ttcol;
	  *cp2++ = *cp1++;
	}
      (*term.t_rev) (FALSE);	 /* turn rev video off */

      /* update the needed flags */
      *flags &= ~VFCHG;
      if (req)
	*flags |= VFREV;
      else
	*flags &= ~VFREV;
      return;
    }

  /* advance past any common chars at the left */
  while (cp1 != &vline[term.t_ncol] && cp1[0] == cp2[0])
    {
      ++cp1;
      ++cp2;
    }

  /* This can still happen, even though we only call this routine on changed
   * lines. A hard update is always done when a line splits, a massive change
   * is done, or a buffer is displayed twice. This optimizes out most of the
   * excess updating. A lot of computes are used, but these tend to be hard
   * operations that do a lot of update, so I don't really care */
  /* if both lines are the same, no update needs to be done */
  if (cp1 == &vline[term.t_ncol])
    return;

  /* find out if there is a match on the right */
  nbflag = FALSE;
  cp3 = &vline[term.t_ncol];
  cp4 = &pline[term.t_ncol];

  while (cp3[-1] == cp4[-1])
    {
      --cp3;
      --cp4;
      if (cp3[0] != ' ')	 /* Note if any nonblank */
	nbflag = TRUE;		 /* in right match */
    }

  cp5 = cp3;

  if (nbflag == FALSE && eolexist == TRUE)
    {				/* Erase to EOL ? */
      while (cp5 != cp1 && cp5[-1] == ' ')
	--cp5;
      if (cp3 - cp5 <= 3)	/* Use only if erase is */
	cp5 = cp3;		/* fewer characters */
    }
  movecursor (row, cp1 - &vline[0]); /* Go to start of line */

  while (cp1 != cp5)
    {				/* Ordinary */
      (*term.t_putchar) (*cp1);
      ++ttcol;
      *cp2++ = *cp1++;
    }

  if (cp5 != cp3)
    {				/* Erase */
      (*term.t_eeol) ();
      while (cp1 != cp3)
	*cp2++ = *cp1++;
    }
  *flags &= ~VFCHG;		/* flag this line is changed */
}

/*
 * Redisplay the mode line for the window pointed to by the "wp". This is the
 * only routine that has any idea of how the modeline is formatted. You can
 * change the modeline format by hacking at this routine. Called by "update"
 * any time there is a dirty window.
 */
void modeline(WINDOW *wp)
{
  BUFFER *bp;
  int n;			/* cursor position count */
  int len;			/* line/column display check */
  char sl[17];			/* line/column display (probably overkill) */

  n = wp->w_toprow + wp->w_ntrows; /* Location */
  vscreen[n]->v_flag |= VFCHG; /* Redraw next time */
  vtmove(n, 0);		       /* Seek to right line */
  bp = wp->w_bufp;
  vtputc('-');
  vtputc('-');
  if ((bp->b_flag & BFCHG) != 0) {	/* "**" if changed */
    vtputc('*');
    vtputc('*');
  } else {
    vtputc('-');
    vtputc('-');
  }
  vtputc('-');
  n = 5;
  n += vtputs("emg: ");
  if (bp->b_fname[0] == '\0')
    n += vtputs("*scratch*");
  else
    n += vtputs(&(bp->b_fname[0]));
  while (n++ < 42)		/* Pad out with blanks.  */
    vtputc(' ');
  n += vtputs("(fundamental)");

  len = snprintf(sl, sizeof(sl), "--L%d--C%d",
		 (wp->w_dotline + 1), getccol(FALSE));
  if (len < sizeof(sl) && len != -1)
    n += vtputs(sl);

  while (n++ <= term.t_ncol)      /* Pad to full width */
      vtputc('-');
}

/* update all the mode lines */
void upmode()
{
  WINDOW *wp;

  wp = wheadp;
  while (wp != NULL)
    {
      wp->w_flag |= WFMODE;
      wp = wp->w_wndp;
    }
}

/*
 * Ask a yes or no question in the message line. Return either TRUE, FALSE, or
 * ABORT. The ABORT status is returned if the user bumps out of the question
 * with a ^G. Used any time a confirmation is required.
 */
int mlyesno(char *prompt)
{
  char c;			/* input character */
  char buf[NPAT];		/* prompt to user */

  for (;;)
    {
      /* build and prompt the user */
      strncpy(buf, prompt, 60);

      strncat(buf, " [y/n]? ", 9);
      mlwrite(buf);

      /* get the responce */
      c = (*term.t_getchar) ();
      if (c == BELL)		 /* Bail out! */
	return (ABORT);
      if (c == 'y' || c == 'Y')
	return (TRUE);
      if (c == 'n' || c == 'N')
	return (FALSE);
    }
}

/* A more generalized prompt/reply function allowing the caller to specify the
 * proper terminator. If the terminator is not a return ('\n') it will echo as
 * "<NL>"
 */
int mlreplyt(char *prompt, char *buf, int nbuf, char eolchar)
{
  int cpos, i, c;

  cpos = 0;

  if (kbdmop != NULL)
    {
      while ((c = *kbdmop++) != '\0')
	buf[cpos++] = c;
      buf[cpos] = 0;
      if (buf[0] == 0)
	return (FALSE);
      return (TRUE);
    }
  mlwrite(prompt);

  for (;;)
    {
      /* get a character from the user. if it is a <ret> change it to a <NL> */
      c = (*term.t_getchar) ();
      if (c == 0x0d)
	c = '\n';

      if (c == eolchar)
	{
	  buf[cpos++] = 0;

	  if (kbdmip != NULL)
	    {
	      if (kbdmip + cpos > &kbdm[NKBDM - 3])
		{
		  ctrlg(FALSE, 0);
		  (*term.t_flush) ();
		  return (ABORT);
		}
	      for (i = 0; i < cpos; ++i)
		*kbdmip++ = buf[i];
	    }
	  (*term.t_putchar) ('\r');
	  ttcol = 0;
	  (*term.t_flush) ();

	  if (buf[0] == 0)
	    return (FALSE);

	  return (TRUE);

	}
      else if (c == 0x07)
	{			     /* Bell, abort */
	  (*term.t_putchar) ('^');
	  (*term.t_putchar) ('G');
	  ttcol += 2;
	  ctrlg (FALSE, 0);
	  (*term.t_flush) ();
	  return (ABORT);
	}
      else if (c == 0x7F || c == 0x08)
	{			       /* rubout/erase */
	  if (cpos != 0)
	    {
	      (*term.t_putchar) ('\b');
	      (*term.t_putchar) (' ');
	      (*term.t_putchar) ('\b');
	      --ttcol;

	      if (buf[--cpos] < 0x20)
		{
		  (*term.t_putchar) ('\b');
		  (*term.t_putchar) (' ');
		  (*term.t_putchar) ('\b');
		  --ttcol;
		}
	      if (buf[cpos] == '\n')
		{
		  (*term.t_putchar) ('\b');
		  (*term.t_putchar) ('\b');
		  (*term.t_putchar) (' ');
		  (*term.t_putchar) (' ');
		  (*term.t_putchar) ('\b');
		  (*term.t_putchar) ('\b');
		  --ttcol;
		  --ttcol;
		}
	      (*term.t_flush) ();
	    }
	}
      else if (c == 0x15)
	{				 /* C-U, kill */
	  while (cpos != 0)
	    {
	      (*term.t_putchar) ('\b');
	      (*term.t_putchar) (' ');
	      (*term.t_putchar) ('\b');
	      --ttcol;
	      if (buf[--cpos] < 0x20)
		{
		  (*term.t_putchar) ('\b');
		  (*term.t_putchar) (' ');
		  (*term.t_putchar) ('\b');
		  --ttcol;
		}
	    }
	  (*term.t_flush) ();
	}
      else
	{
	  if (cpos < nbuf - 1)
	    {
	      buf[cpos++] = c;
	      if ((c < ' ') && (c != '\n'))
		{
		  (*term.t_putchar) ('^');
		  ++ttcol;
		  c ^= 0x40;
		}
	      if (c != '\n')
		(*term.t_putchar) (c);
	      else
		{		/* put out <NL> for <ret> */
		  (*term.t_putchar) ('<');
		  (*term.t_putchar) ('N');
		  (*term.t_putchar) ('L');
		  (*term.t_putchar) ('>');
		  ttcol += 3;
		}
	      ++ttcol;
	      (*term.t_flush) ();
	    }
	}
    }
}

/*
 * Write a prompt into the message line, then read back a response. Keep track
 * of the physical position of the cursor. If we are in a keyboard macro throw
 * the prompt away, and return the remembered response. This lets macros run
 * at full speed. The reply is always terminated by a carriage return. Handle
 * erase, kill, and abort keys.
 */
int mlreply(char *prompt, char *buf, int nbuf)
{
  return(mlreplyt (prompt, buf, nbuf, '\n'));
}

/*
 * Write a message into the message line. Keep track of the physical cursor
 * position. A small class of printf like format items is handled. Assumes the
 * stack grows down; this assumption is made by the "++" in the argument scan
 * loop. Set the "message line" flag TRUE.
 */
void mlwrite(char *fmt, int arg)
{
  int c;
  char *ap;

  if (eolexist == FALSE)
    {
      mlerase();
      (*term.t_flush) ();
    }
  movecursor(term.t_nrow, 0);
  ap = (char *) &arg;
  while ((c = *fmt++) != 0)
    {
      if (c != '%')
	{
	  (*term.t_putchar) (c);
	  ++ttcol;
	}
      else
	{
	  c = *fmt++;
	  switch (c)
	    {
	    case 'd':
	      mlputi(*(int *) ap, 10);
	      ap += sizeof(int);
	      break;

	    case 'o':
	      mlputi(*(int *) ap, 8);
	      ap += sizeof(int);
	      break;

	    case 'x':
	      mlputi(*(int *) ap, 16);
	      ap += sizeof(int);
	      break;

	    case 'D':
	      mlputli(*(long *) ap, 10);
	      ap += sizeof(long);
	      break;

	    case 's':
	      mlputs(*(char **) &ap);
	      ap += sizeof(char *);
	      break;

	    case 'c':
	      (*term.t_putchar) (*ap);
	      ++ttcol;
	      ap += sizeof(char *);
	      break;

	    default:
	      (*term.t_putchar) (c);
	      ++ttcol;
	    }
	}
    }
  if (eolexist == TRUE)
    (*term.t_eeol) ();
  (*term.t_flush) ();
  mpresf = TRUE;
}

/*
 * Write out a string. Update the physical cursor position. This assumes that
 * the characters in the string all have width "1"; if this is not the case
 * things will get screwed up a little.
 */
void mlputs(char *s)
{
  int c;

  while ((c = *s++) != 0)
    {
      (*term.t_putchar) (c);
      ++ttcol;
    }
}

/*
 * Write out an integer, in the specified radix. Update the physical cursor
 * position. This will not handle any negative numbers; maybe it should.
 */
void mlputi(int i, int r)
{
  int q;
  static char hexdigits[] = "0123456789abcdef";

  if (i < 0)
    {
      i = -i;
      (*term.t_putchar) ('-');
    }
  q = i / r;

  if (q != 0)
    mlputi(q, r);

  (*term.t_putchar) (hexdigits[i % r]);
  ++ttcol;
}

/*
 * do the same except as a long integer.
 */
void mlputli(long l, int r)
{
  long q;

  if (l < 0)
    {
      l = -l;
      (*term.t_putchar) ('-');
    }
  q = l / r;

  if (q != 0)
    mlputli(q, r);

  (*term.t_putchar) ((int) (l % r) + '0');
  ++ttcol;
}
