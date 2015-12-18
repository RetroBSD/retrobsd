/* This file is in the public domain. */

/*
 * The functions in this file are a general set of line management utilities.
 * They are the only routines that touch the text. They also touch the buffer
 * and window structures, to make sure that the necessary updating gets done.
 * There are routines in this file that handle the kill buffer too. It isn't
 * here for any good reason.
 */

#include <stdlib.h>		/* malloc(3) */
#include "estruct.h"
#include "edef.h"

extern void mlwrite();
extern int backchar(int, int);

LINE* lalloc(int);
void lfree(LINE *);
void lchange(int);
int linsert(int, int);
int lnewline();
int ldelete(int, int);
int ldelnewline();
void kdelete();
int kinsert(int);
int kremove(int);

#define NBLOCK	16		/* Line block chunk size */
#define KBLOCK	1024		/* Kill buffer block size */

char *kbufp = NULL;		/* Kill buffer data */
unsigned long kused = 0;	/* # of bytes used in KB */
unsigned long ksize = 0;	/* # of bytes allocated in KB */

/*
 * This routine allocates a block of memory large enough to hold a LINE
 * containing "used" characters. The block is always rounded up a bit. Return
 * a pointer to the new block, or NULL if there isn't any memory left. Print a
 * message in the message line if no space.
 */
LINE *
lalloc(int used)
{
  LINE *lp;
  int size;

  size = (used + NBLOCK - 1) & ~(NBLOCK - 1);
  if (size == 0)	       /* Assume that an empty */
    size = NBLOCK;	       /* line is for type-in */
  if ((lp = (LINE *) malloc(sizeof(LINE) + size)) == NULL)
    {
      mlwrite("Cannot allocate %d bytes", size);
      return (NULL);
    }
  lp->l_size = size;
  lp->l_used = used;
  return (lp);
}

/*
 * Delete line "lp". Fix all of the links that might point at it (they are
 * moved to offset 0 of the next line. Unlink the line from whatever buffer it
 * might be in. Release the memory. The buffers are updated too; the magic
 * conditions described in the above comments don't hold here
 */
void
lfree(LINE *lp)
{
  BUFFER *bp;
  WINDOW *wp;

  wp = wheadp;
  while (wp != NULL)
    {
      if (wp->w_linep == lp)
	wp->w_linep = lp->l_fp;
      if (wp->w_dotp == lp)
	{
	  wp->w_dotp = lp->l_fp;
	  wp->w_doto = 0;
	}
      if (wp->w_markp == lp)
	{
	  wp->w_markp = lp->l_fp;
	  wp->w_marko = 0;
	}
      wp = wp->w_wndp;
    }
  bp = bheadp;
  while (bp != NULL) {
    if (bp->b_dotp == lp) {
      bp->b_dotp = lp->l_fp;
      bp->b_doto = 0;
    }
    if (bp->b_markp == lp) {
      bp->b_markp = lp->l_fp;
      bp->b_marko = 0;
    }
    bp = bp->b_bufp;
  }
  lp->l_bp->l_fp = lp->l_fp;
  lp->l_fp->l_bp = lp->l_bp;
  free((char *) lp);
}

/*
 * This routine gets called when a character is changed in place in the
 * current buffer. It updates all of the required flags in the buffer and
 * window system. The flag used is passed as an argument. Set MODE if the
 * mode line needs to be updated (the "**" has to be set).
 */
void
lchange(int flag)
{
  WINDOW *wp;

  if ((curbp->b_flag & BFCHG) == 0) {	/* First change, so */
    flag |= WFMODE;			/* update mode lines */
    curbp->b_flag |= BFCHG;
  }
  wp = wheadp;
  while (wp != NULL) {
    if (wp->w_bufp == curbp)
      wp->w_flag |= flag;
    wp = wp->w_wndp;
  }
}

/*
 * Insert "n" copies of the character "c" at the current location of dot. In
 * the easy case all that happens is the text is stored in the line. In the
 * hard case, the line has to be reallocated. When the window list is updated,
 * take special care; I screwed it up once. You always update dot in the
 * current window. You update mark, and a dot in another window, if it is
 * greater than the place where you did the insert. Return TRUE if all is
 * well, and FALSE on errors
 */
int
linsert(int n, int c)
{
  WINDOW *wp;
  LINE *lp1, *lp2, *lp3;
  char *cp1, *cp2;
  int i, doto;

  lchange(WFEDIT);
  lp1 = curwp->w_dotp;	       /* Current line */
  if (lp1 == curbp->b_linep)
    {			       /* At the end: special */
      if (curwp->w_doto != 0)
	{
	  mlwrite("Bug: linsert");
	  return (FALSE);
	}
      if ((lp2 = lalloc(n)) == NULL)	/* Allocate new line */
	return (FALSE);
      lp3 = lp1->l_bp;	       /* Previous line */
      lp3->l_fp = lp2;	       /* Link in */
      lp2->l_fp = lp1;
      lp1->l_bp = lp2;
      lp2->l_bp = lp3;
      for (i = 0; i < n; ++i)
	lp2->l_text[i] = c;
      curwp->w_dotp = lp2;
      curwp->w_doto = n;
      return (TRUE);
    }
  doto = curwp->w_doto;	       /* Save for later */
  if (lp1->l_used + n > lp1->l_size)
    {			       /* Hard: reallocate */
      if ((lp2 = lalloc(lp1->l_used + n)) == NULL)
	return (FALSE);
      cp1 = &lp1->l_text[0];
      cp2 = &lp2->l_text[0];
      while (cp1 != &lp1->l_text[doto])
	*cp2++ = *cp1++;
      cp2 += n;
      while (cp1 != &lp1->l_text[lp1->l_used])
	*cp2++ = *cp1++;
      lp1->l_bp->l_fp = lp2;
      lp2->l_fp = lp1->l_fp;
      lp1->l_fp->l_bp = lp2;
      lp2->l_bp = lp1->l_bp;
      free((char *) lp1);
    }
  else
    {				/* Easy: in place */
      lp2 = lp1;		/* Pretend new line */
      lp2->l_used += n;
      cp2 = &lp1->l_text[lp1->l_used];
      cp1 = cp2 - n;
      while (cp1 != &lp1->l_text[doto])
	*--cp2 = *--cp1;
    }
  for (i = 0; i < n; ++i)	/* Add the characters */
    lp2->l_text[doto + i] = c;
  wp = wheadp;		       /* Update windows */
  while (wp != NULL)
    {
      if (wp->w_linep == lp1)
	wp->w_linep = lp2;
      if (wp->w_dotp == lp1)
	{
	  wp->w_dotp = lp2;
	  if (wp == curwp || wp->w_doto > doto)
	    wp->w_doto += n;
	}
      if (wp->w_markp == lp1)
	{
	  wp->w_markp = lp2;
	  if (wp->w_marko > doto)
	    wp->w_marko += n;
	}
      wp = wp->w_wndp;
    }
  return (TRUE);
}

/*
 * Insert a newline into the buffer at the current location of dot in the
 * current window. The funny ass-backwards way it does things is not a botch;
 * it just makes the last line in the file not a special case. Return TRUE if
 * everything works out and FALSE on error (memory allocation failure). The
 * update of dot and mark is a bit easier then in the above case, because the
 * split forces more updating.
 */
int
lnewline()
{
  WINDOW *wp;
  char *cp1, *cp2;
  LINE *lp1, *lp2;
  int doto;

  lchange(WFHARD);

  curwp->w_bufp->b_lines++;

  lp1 = curwp->w_dotp;	       /* Get the address and */
  doto = curwp->w_doto;	       /* offset of "." */
  if ((lp2 = lalloc(doto)) == NULL)	/* New first half line */
    return (FALSE);
  cp1 = &lp1->l_text[0];       /* Shuffle text around */
  cp2 = &lp2->l_text[0];
  while (cp1 != &lp1->l_text[doto])
    *cp2++ = *cp1++;
  cp2 = &lp1->l_text[0];
  while (cp1 != &lp1->l_text[lp1->l_used])
    *cp2++ = *cp1++;
  lp1->l_used -= doto;
  lp2->l_bp = lp1->l_bp;
  lp1->l_bp = lp2;
  lp2->l_bp->l_fp = lp2;
  lp2->l_fp = lp1;
  wp = wheadp;		       /* Windows */
  while (wp != NULL)
    {
      if (wp->w_linep == lp1)
	wp->w_linep = lp2;
      if (wp->w_dotp == lp1)
	{
	  if (wp->w_doto < doto)
	    wp->w_dotp = lp2;
	  else
	    wp->w_doto -= doto;
	}
      if (wp->w_markp == lp1)
	{
	  if (wp->w_marko < doto)
	    wp->w_markp = lp2;
	  else
	    wp->w_marko -= doto;
	}
      wp = wp->w_wndp;
    }
  curwp->w_dotline++;
  return (TRUE);
}

/*
 * This function deletes "n" bytes, starting at dot. It understands how do
 * deal with end of lines, etc. It returns TRUE if all of the characters were
 * deleted, and FALSE if they were not (because dot ran into the end of the
 * buffer. The "kflag" is TRUE if the text should be put in the kill buffer.
 */
int
ldelete(int n, int kflag)
{
  LINE *dotp;
  WINDOW *wp;
  char *cp1, *cp2;
  int doto, chunk;

  while (n != 0)
    {
      dotp = curwp->w_dotp;
      doto = curwp->w_doto;
      if (dotp == curbp->b_linep) /* Hit end of buffer */
	return (FALSE);
      chunk = dotp->l_used - doto; /* Size of chunk */
      if (chunk > n)
	chunk = n;
      if (chunk == 0)
	{			/* End of line, merge */
	  lchange(WFHARD);
	  if (ldelnewline() == FALSE
	      || (kflag != FALSE && kinsert('\n') == FALSE))
	    return (FALSE);
	  --n;
	  continue;
	}
      lchange(WFEDIT);
      cp1 = &dotp->l_text[doto]; /* Scrunch text */
      cp2 = cp1 + chunk;
      if (kflag != FALSE)
	{			/* Kill? */
	  while (cp1 != cp2)
	    {
	      if (kinsert (*cp1) == FALSE)
		return (FALSE);
	      ++cp1;
	    }
	  cp1 = &dotp->l_text[doto];
	}
      while (cp2 != &dotp->l_text[dotp->l_used])
	*cp1++ = *cp2++;
      dotp->l_used -= chunk;
      wp = wheadp;		/* Fix windows */
      while (wp != NULL)
	{
	  if (wp->w_dotp == dotp && wp->w_doto >= doto)
	    {
	      wp->w_doto -= chunk;
	      if (wp->w_doto < doto)
		wp->w_doto = doto;
	    }
	  if (wp->w_markp == dotp && wp->w_marko >= doto)
	    {
	      wp->w_marko -= chunk;
	      if (wp->w_marko < doto)
		wp->w_marko = doto;
	    }
	  wp = wp->w_wndp;
	}
      n -= chunk;
    }
  return (TRUE);
}

/*
 * Delete a newline. Join the current line with the next line. If the next
 * line is the magic header line always return TRUE; merging the last line
 * with the header line can be thought of as always being a successful
 * operation, even if nothing is done, and this makes the kill buffer work
 * "right". Easy cases can be done by shuffling data around. Hard cases
 * require that lines be moved about in memory. Return FALSE on error and TRUE
 * if all looks ok. Called by "ldelete" only.
 */
int
ldelnewline()
{
  LINE *lp1, *lp2, *lp3;
  WINDOW *wp;
  char *cp1, *cp2;

  lp1 = curwp->w_dotp;
  lp2 = lp1->l_fp;
  if (lp2 == curbp->b_linep)
    {			       /* At the buffer end */
      if (lp1->l_used == 0)	 /* Blank line */
	lfree(lp1);
      return (TRUE);
    }
  /* Keep line counts in sync */
  curwp->w_bufp->b_lines--;
  if (lp2->l_used <= lp1->l_size - lp1->l_used)
    {
      cp1 = &lp1->l_text[lp1->l_used];
      cp2 = &lp2->l_text[0];
      while (cp2 != &lp2->l_text[lp2->l_used])
	*cp1++ = *cp2++;
      wp = wheadp;
      while (wp != NULL)
	{
	  if (wp->w_linep == lp2)
	    wp->w_linep = lp1;
	  if (wp->w_dotp == lp2)
	    {
	      wp->w_dotp = lp1;
	      wp->w_doto += lp1->l_used;
	    }
	  if (wp->w_markp == lp2)
	    {
	      wp->w_markp = lp1;
	      wp->w_marko += lp1->l_used;
	    }
	  wp = wp->w_wndp;
	}
      lp1->l_used += lp2->l_used;
      lp1->l_fp = lp2->l_fp;
      lp2->l_fp->l_bp = lp1;
      free((char *) lp2);
      return (TRUE);
    }
  if ((lp3 = lalloc(lp1->l_used + lp2->l_used)) == NULL)
    return (FALSE);
  cp1 = &lp1->l_text[0];
  cp2 = &lp3->l_text[0];
  while (cp1 != &lp1->l_text[lp1->l_used])
    *cp2++ = *cp1++;
  cp1 = &lp2->l_text[0];
  while (cp1 != &lp2->l_text[lp2->l_used])
    *cp2++ = *cp1++;
  lp1->l_bp->l_fp = lp3;
  lp3->l_fp = lp2->l_fp;
  lp2->l_fp->l_bp = lp3;
  lp3->l_bp = lp1->l_bp;
  wp = wheadp;
  while (wp != NULL)
    {
      if (wp->w_linep == lp1 || wp->w_linep == lp2)
	wp->w_linep = lp3;
      if (wp->w_dotp == lp1)
	wp->w_dotp = lp3;
      else if (wp->w_dotp == lp2)
	{
	  wp->w_dotp = lp3;
	  wp->w_doto += lp1->l_used;
	}
      if (wp->w_markp == lp1)
	wp->w_markp = lp3;
      else if (wp->w_markp == lp2)
	{
	  wp->w_markp = lp3;
	  wp->w_marko += lp1->l_used;
	}
      wp = wp->w_wndp;
    }
  free((char *) lp1);
  free((char *) lp2);
  return (TRUE);
}

/*
 * Delete all of the text saved in the kill buffer. Called by commands when a
 * new kill context is being created. The kill buffer array is released, just
 * in case the buffer has grown to immense size. No errors.
 */
void
kdelete()
{
  if (kbufp != NULL) {
      free((char *) kbufp);
      kbufp = NULL;
      kused = 0;
      ksize = 0;
    }
}

/*
 * Insert a character to the kill buffer, enlarging the buffer if there isn't
 * any room. Always grow the buffer in chunks, on the assumption that if you
 * put something in the kill buffer you are going to put more stuff there too
 * later. Return TRUE if all is well, and FALSE on errors.
 */
int
kinsert(int c)
{
  char *nbufp;

  if (kused == ksize)
    {
      if (ksize == 0)	       /* first time through? */
	nbufp = malloc(KBLOCK); /* alloc the first block */
      else		       /* or re allocate a bigger block */
	nbufp = realloc(kbufp, ksize + KBLOCK);
      if (nbufp == NULL)	       /* abort if it fails */
	return (FALSE);
      kbufp = nbufp;	       /* point our global at it */
      ksize += KBLOCK;	       /* and adjust the size */
    }
  kbufp[kused++] = c;
  return (TRUE);
}

/*
 * This function gets characters from the kill buffer. If the character index
 * "n" is off the end, it returns "-1". This lets the caller just scan along
 * until it gets a "-1" back.
 */
int
kremove(int n)
{
  if (n >= kused)
    return (-1);
  else
    return (kbufp[n] & 0xFF);
}
