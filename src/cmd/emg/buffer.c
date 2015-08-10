/* This file is in the public domain. */

/*
 * Buffer management. Some of the functions are internal, and some are actually
 * attached to user keys. Like everyone else, they set hints for the display
 * system.
 */

#include <stdlib.h>		/* free(3), malloc(3) */
#include <string.h>		/* strncpy(3) */
#include "estruct.h"
#include "edef.h"

extern int mlreply(char *prompt, char *buf, int nbuf);
extern int readin(char fname[]);
extern void mlwrite();
extern void mlerase();
extern int mlyesno(char *prompt);
extern void lfree(LINE *lp);
extern LINE *lalloc();

int swbuffer(BUFFER *bp);
int addline(char *text);
int anycb();
BUFFER* bfind(char *bname, int cflag, int bflag);
int bclear(BUFFER *bp);

/*
 * make buffer BP current
 */
int swbuffer(BUFFER *bp)
{
  WINDOW *wp;

  if (--curbp->b_nwnd == 0)
    {				/* Last use. */
      curbp->b_dotp = curwp->w_dotp;
      curbp->b_doto = curwp->w_doto;
      curbp->b_markp = curwp->w_markp;
      curbp->b_marko = curwp->w_marko;
    }
  curbp = bp;			/* Switch. */
  if (curbp->b_active != TRUE)
    {				/* buffer not active yet */
      /* read it in and activate it */
      readin(curbp->b_fname);
      curbp->b_dotp = lforw(curbp->b_linep);
      curbp->b_doto = 0;
      curbp->b_active = TRUE;
    }
  curwp->w_bufp = bp;
  curwp->w_linep = bp->b_linep;	/* For macros, ignored */
  curwp->w_flag |= WFMODE | WFFORCE | WFHARD; /* Quite nasty */
  if (bp->b_nwnd++ == 0)
    {				/* First use */
      curwp->w_dotp = bp->b_dotp;
      curwp->w_doto = bp->b_doto;
      curwp->w_markp = bp->b_markp;
      curwp->w_marko = bp->b_marko;
      return (TRUE);
    }
  wp = wheadp;			/* Look for old */
  while (wp != 0)
    {
      if (wp != curwp && wp->w_bufp == bp)
	{
	  curwp->w_dotp = wp->w_dotp;
	  curwp->w_doto = wp->w_doto;
	  curwp->w_markp = wp->w_markp;
	  curwp->w_marko = wp->w_marko;
	  break;
	}
      wp = wp->w_wndp;
    }
  return (TRUE);
}

/*
 * The argument "text" points to a string. Append this line to the buffer list
 * buffer. Handcraft the EOL on the end. Return TRUE if it worked and FALSE if
 * you ran out of room.
 */
int addline(char *text)
{
  LINE *lp;
  int ntext, i;

  ntext = strlen(text);
  if ((lp = lalloc(ntext)) == NULL)
    return (FALSE);
  for (i = 0; i < ntext; ++i)
    lputc(lp, i, text[i]);
  blistp->b_linep->l_bp->l_fp = lp; /* Hook onto the end */
  lp->l_bp = blistp->b_linep->l_bp;
  blistp->b_linep->l_bp = lp;
  lp->l_fp = blistp->b_linep;
  if (blistp->b_dotp == blistp->b_linep) /* If "." is at the end */
    blistp->b_dotp = lp;	/* move it to new line */
  return (TRUE);
}

/*
 * Look through the list of buffers. Return TRUE if there are any changed
 * buffers. Buffers that hold magic internal stuff are not considered; who
 * cares if the list of buffer names is hacked. Return FALSE if no buffers
 * have been changed.
 */
int anycb()
{
  BUFFER *bp;

  bp = bheadp;
  while (bp != NULL)
    {
      if ((bp->b_flag & BFTEMP) == 0 && (bp->b_flag & BFCHG) != 0)
	return (TRUE);
      bp = bp->b_bufp;
    }
  return (FALSE);
}

/*
 * Find a buffer, by name. Return a pointer to the BUFFER structure associated
 * with it. If the named buffer is found, but is a TEMP buffer (like the
 * buffer list) conplain. If the buffer is not found and the "cflag" is TRUE,
 * create it. The "bflag" is the settings for the flags in in buffer.
 */
BUFFER* bfind(char *bname, int cflag, int bflag)
{
  BUFFER *bp, *sb;
  LINE *lp;

  bp = bheadp;
  while (bp != 0)
    {
      if (strcmp(bname, bp->b_bname) == 0)
	{
	  if ((bp->b_flag & BFTEMP) != 0)
	    {
	      mlwrite ("Cannot select builtin buffer");
	      return (0);
	    }
	  return (bp);
	}
      bp = bp->b_bufp;
    }
  if (cflag != FALSE)
    {
      if ((bp = (BUFFER *) malloc(sizeof(BUFFER))) == NULL)
	return (0);
      if ((lp = lalloc(0)) == NULL)
	{
	  free(bp);
	  return (BUFFER*)0;
	}
      /* find the place in the list to insert this buffer */
      if (bheadp == NULL || strcmp(bheadp->b_bname, bname) > 0)
	{
	  /* insert at the begining */
	  bp->b_bufp = bheadp;
	  bheadp = bp;
	}
      else
	{
	  sb = bheadp;
	  while (sb->b_bufp != 0)
	    {
	      if (strcmp(sb->b_bufp->b_bname, bname) > 0)
		break;
	      sb = sb->b_bufp;
	    }

	  /* and insert it */
	  bp->b_bufp = sb->b_bufp;
	  sb->b_bufp = bp;
	}

      /* and set up the other buffer fields */
      bp->b_active = TRUE;
      bp->b_dotp = lp;
      bp->b_doto = 0;
      bp->b_markp = 0;
      bp->b_marko = 0;
      bp->b_flag = (char)bflag;
      bp->b_nwnd = 0;
      bp->b_linep = lp;
      bp->b_lines = 1;
      strncpy(bp->b_fname, "", 1);
      strncpy(bp->b_bname, bname, NBUFN);
      lp->l_fp = lp;
      lp->l_bp = lp;
    }
  return (bp);
}

/*
 * This routine blows away all of the text in a buffer. If the buffer is
 * marked as changed then we ask if it is ok to blow it away; this is to save
 * the user the grief of losing text. The window chain is nearly always wrong
 * if this gets called; the caller must arrange for the updates that are
 * required. Return TRUE if everything looks good.
 */
int bclear(BUFFER *bp)
{
  LINE *lp;
  int s;

  if ((bp->b_flag & BFTEMP) == 0 /* Not scratch buffer */
      && (bp->b_flag & BFCHG) != 0 /* Something changed */
      && (s = mlyesno("Discard changes")) != TRUE)
    return (s);
  bp->b_flag &= ~BFCHG;		/* Not changed */
  while ((lp = lforw(bp->b_linep)) != bp->b_linep)
    lfree(lp);
  bp->b_dotp = bp->b_linep;	/* Fix "." */
  bp->b_doto = 0;
  bp->b_markp = 0;		/* Invalidate "mark" */
  bp->b_marko = 0;
  bp->b_lines = 1;
  return (TRUE);
}
