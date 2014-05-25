/* This file is in the public domain. */

/*
 * The routines in this file implement commands that work word at a time.
 * There are all sorts of word mode commands. If I do any sentence and/or
 * paragraph mode commands, they are likely to be put in this file
 */

#include "estruct.h"
#include "edef.h"

extern int backchar(int f, int n);
extern int forwchar(int f, int n);
extern void lchange(int flag);
extern int ldelete(int n, int kflag);
extern void mlwrite();
extern int linsert(int n, int c);
extern int lnewline();

int backword(int f, int n);
int forwword(int f, int n);
int upperword(int f, int n);
int lowerword(int f, int n);
int capword(int f, int n);
int delfword(int f, int n);
int delbword(int f, int n);
int inword();

/*
 * Move the cursor backward by "n" words. All of the details of motion are
 * performed by the "backchar" and "forwchar" routines. Error if you try to
 * move beyond the buffers
 */
int backword(int f, int n)
{
  if (n < 0)
    return (forwword(f, -n));
  if (backchar(FALSE, 1) == FALSE)
    return (FALSE);
  while (n--)
    {
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
int forwword(int f, int n)
{
  if (n < 0)
    return (backword(f, -n));
  while (n--)
    {
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
 * Move the cursor forward by the specified number of words. As you move,
 * convert any characters to upper case. Error if you try and move beyond the
 * end of the buffer. Bound to "M-U"
 */
int upperword(int f, int n)
{
  int c;

  if (n < 0)
    return (FALSE);
  while (n--)
    {
      while (inword() == FALSE)
	{
	  if (forwchar(FALSE, 1) == FALSE)
	    return (FALSE);
	}
      while (inword() != FALSE)
	{
	  c = lgetc(curwp->w_dotp, curwp->w_doto);
	  if (c >= 'a' && c <= 'z')
	    {
	      c -= 'a' - 'A';
	      lputc(curwp->w_dotp, curwp->w_doto, c);
	      lchange(WFHARD);
	    }
	  if (forwchar(FALSE, 1) == FALSE)
	    return (FALSE);
	}
    }
  return (TRUE);
}

/*
 * Move the cursor forward by the specified number of words. As you move
 * convert characters to lower case. Error if you try and move over the end of
 * the buffer. Bound to "M-L"
 */
int lowerword(int f, int n)
{
  int c;

  if (n < 0)
    return (FALSE);
  while (n--)
    {
      while (inword() == FALSE)
	{
	  if (forwchar(FALSE, 1) == FALSE)
	    return (FALSE);
	}
      while (inword() != FALSE)
	{
	  c = lgetc(curwp->w_dotp, curwp->w_doto);
	  if (c >= 'A' && c <= 'Z')
	    {
	      c += 'a' - 'A';
	      lputc(curwp->w_dotp, curwp->w_doto, c);
	      lchange(WFHARD);
	    }
	  if (forwchar(FALSE, 1) == FALSE)
	    return (FALSE);
	}
    }
  return (TRUE);
}

/*
 * Move the cursor forward by the specified number of words. As you move
 * convert the first character of the word to upper case, and subsequent
 * characters to lower case. Error if you try and move past the end of the
 * buffer. Bound to "M-C"
 */
int capword(int f, int n)
{
  int c;

  if (n < 0)
    return(FALSE);
  while (n--)
    {
      while (inword() == FALSE)
	{
	  if (forwchar(FALSE, 1) == FALSE)
	    return (FALSE);
	}
      if (inword() != FALSE)
	{
	  c = lgetc(curwp->w_dotp, curwp->w_doto);
	  if (c >= 'a' && c <= 'z')
	    {
	      c -= 'a' - 'A';
	      lputc(curwp->w_dotp, curwp->w_doto, c);
	      lchange(WFHARD);
	    }
	  if (forwchar(FALSE, 1) == FALSE)
	    return (FALSE);
	  while (inword() != FALSE)
	    {
	      c = lgetc(curwp->w_dotp, curwp->w_doto);
	      if (c >= 'A' && c <= 'Z')
		{
		  c += 'a' - 'A';
		  lputc(curwp->w_dotp, curwp->w_doto, c);
		  lchange(WFHARD);
		}
	      if (forwchar(FALSE, 1) == FALSE)
		return (FALSE);
	    }
	}
    }
  return (TRUE);
}

/*
 * Kill forward by "n" words. Remember the location of dot. Move forward by
 * the right number of words. Put dot back where it was and issue the kill
 * command for the right number of characters. Bound to "M-D"
 */
int delfword(int f, int n)
{
  LINE *dotp;
  int size, doto;

  if (n < 0)
    return (FALSE);
  dotp = curwp->w_dotp;
  doto = curwp->w_doto;
  size = 0;
  while (n--)
    {
      while (inword() != FALSE)
	{
	  if (forwchar(FALSE, 1) == FALSE)
	    return (FALSE);
	  ++size;
	}
      while (inword() == FALSE)
	{
	  if (forwchar(FALSE, 1) == FALSE)
	    return (FALSE);
	  ++size;
	}
    }
  curwp->w_dotp = dotp;
  curwp->w_doto = doto;
  return (ldelete(size, TRUE));
}

/*
 * Kill backwards by "n" words. Move backwards by the desired number of words,
 * counting the characters. When dot is finally moved to its resting place,
 * fire off the kill command. Bound to "M-Rubout" and to "M-Backspace"
 */
int delbword(int f, int n)
{
  int size;

  if (n < 0)
    return (FALSE);
  if (backchar(FALSE, 1) == FALSE)
    return (FALSE);
  size = 0;
  while (n--)
    {
      while (inword() == FALSE)
	{
	  if (backchar(FALSE, 1) == FALSE)
	    return (FALSE);
	  ++size;
	}
      while (inword() != FALSE)
	{
	  if (backchar(FALSE, 1) == FALSE)
	    return (FALSE);
	  ++size;
	}
    }
  if (forwchar(FALSE, 1) == FALSE)
    return (FALSE);
  return (ldelete(size, TRUE));
}

/*
 * Return TRUE if the character at dot is a character that is considered to be
 * part of a word. The word character list is hard coded. Should be setable
 */
int inword()
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
