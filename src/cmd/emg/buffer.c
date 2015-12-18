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

extern int mlreply(char *, char *, int);
extern int readin(char []);
extern void mlwrite();
extern void mlerase();
extern int mlyesno(char *);
extern void lfree(LINE *);
extern LINE *lalloc();

int anycb(void);
BUFFER *bfind(char *);
int bclear(BUFFER *);

/*
 * Look through the list of buffers. Return TRUE if there are any changed
 * buffers. Buffers that hold magic internal stuff are not considered; who
 * cares if the list of buffer names is hacked. Return FALSE if no buffers
 * have been changed.
 */
int anycb(void)
{
  BUFFER *bp;

  bp = bheadp;
  while (bp != NULL) {
    if ((bp->b_flag & BFTEMP) == 0 && (bp->b_flag & BFCHG) != 0)
      return (TRUE);
    bp = bp->b_bufp;
  }
  return (FALSE);
}

/*
 * Find a buffer, by name. Return a pointer to the BUFFER structure associated
 * with it. If the named buffer is found, but is a TEMP buffer (like the
 * buffer list), complain.
 */
BUFFER *
bfind(char *fname)
{
  BUFFER *bp, *sb;
  LINE *lp;

  bp = bheadp;
  while (bp != 0) {
    if (strcmp(fname, bp->b_fname) == 0) {
      if ((bp->b_flag & BFTEMP) != 0) {
	mlwrite("Cannot select builtin buffer");
	return (FALSE);
      }
      return (bp);
    }
    bp = bp->b_bufp;
  }
  if ((bp = (BUFFER *) malloc(sizeof(BUFFER))) == NULL)
    return (FALSE);
  if ((lp = lalloc(0)) == NULL) {
    free(bp);
    return (BUFFER *)0;
  }
  /* find the place in the list to insert this buffer */
  if (bheadp == NULL || strcmp(bheadp->b_fname, fname) > 0) {
    /* insert at the begining */
    bp->b_bufp = bheadp;
    bheadp = bp;
  } else {
    sb = bheadp;
    while (sb->b_bufp != 0) {
      if (strcmp(sb->b_bufp->b_fname, fname) > 0)
	break;
      sb = sb->b_bufp;
    }

    /* and insert it */
    bp->b_bufp = sb->b_bufp;
    sb->b_bufp = bp;
  }

  /* and set up the other buffer fields */
  bp->b_dotp = lp;
  bp->b_doto = 0;
  bp->b_markp = 0;
  bp->b_marko = 0;
  bp->b_flag = 0;
  bp->b_linep = lp;
  bp->b_lines = 1;
  lp->l_fp = lp;
  lp->l_bp = lp;
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
