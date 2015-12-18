/* This file is in the public domain. */

/*
 * The routines in this file implement commands that work word at a time.
 * There are all sorts of word mode commands. If I do any sentence and/or
 * paragraph mode commands, they are likely to be put in this file
 */

#include "estruct.h"
#include "edef.h"

extern int backchar(int, int);
extern int forwchar(int, int);

int backword(int, int);
int forwword(int, int);
int inword(void);

/*
 * Move the cursor backward by "n" words. All of the details of motion are
 * performed by the "backchar" and "forwchar" routines. Error if you try to
 * move beyond the buffers
 */
int
backword(int f, int n)
{
  if (n < 0)
    return (forwword(f, -n));
  if (backchar(FALSE, 1) == FALSE)
    return (FALSE);
  while (n--) {
      while (inword() == FALSE)
	{
	  if (backchar(FALSE, 1) == FALSE)
	    return (FALSE);
	}
      while (inword() != FALSE)
	{
	  if (backchar(FALSE, 1) == FALSE)
	    return (FALSE);
	}
    }
  return (forwchar(FALSE, 1));
}

/*
 * Move the cursor forward by the specified number of words. All of the motion
 * is done by "forwchar". Error if you try and move beyond the buffer's end
 */
int
forwword(int f, int n)
{
  if (n < 0)
    return (backword(f, -n));
  while (n--) {
      while (inword() != FALSE)
	{
	  if (forwchar(FALSE, 1) == FALSE)
	    return (FALSE);
	}
      while (inword() == FALSE)
	{
	  if (forwchar(FALSE, 1) == FALSE)
	    return (FALSE);
	}
    }
  return (TRUE);
}

/*
 * Return TRUE if the character at dot is a character that is considered to be
 * part of a word. The word character list is hard coded. Should be setable
 */
int
inword(void)
{
  int c;

  if (curwp->w_doto == llength(curwp->w_dotp))
    return (FALSE);
  c = lgetc(curwp->w_dotp, curwp->w_doto);
  if (c >= 'a' && c <= 'z')
    return (TRUE);
  if (c >= 'A' && c <= 'Z')
    return (TRUE);
  if (c >= '0' && c <= '9')
    return (TRUE);
  if (c == '$' || c == '_')	/* For identifiers */
    return (TRUE);
  return (FALSE);
}
