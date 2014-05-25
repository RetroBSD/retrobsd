/* This file is in the public domain. */

/*
 * The functions in this file implement commands that search in the forward
 * and backward directions. There are no special characters in the search
 * strings
 */

#include <string.h>		/* strncpy(3), strncat(3) */
#include "estruct.h"
#include "edef.h"

extern void mlwrite();
extern int mlreplyt(char *prompt, char *buf, int nbuf, char eolchar);
extern void update();
extern int forwchar(int f, int n);
extern int ldelete(int n, int kflag);
extern int lnewline();
extern int linsert(int n, int c);

int forwsearch(int f, int n);
int forwhunt(int f, int n);
int backsearch(int f, int n);
int backhunt(int f, int n);
int bsearch(int f, int n);
int eq(int bc, int pc);
int readpattern(char *prompt);
int sreplace(int f, int n);
int qreplace(int f, int n);
int replaces(int kind, int f, int n);
int forscan(char *patrn, int leavep);
void expandp(char *srcstr, char *deststr, int maxlength);

#define	PTBEG	1		/* leave the point at the begining on search */
#define	PTEND	2		/* leave the point at the end on search */

/*
 * Search forward. Get a search string from the user, and search, beginning at
 * ".", for the string. If found, reset the "." to be just after the match
 * string, and [perhaps] repaint the display. Bound to "C-S"
 */
int forwsearch(int f, int n)
{
  int status;

  if (n == 0)			/* resolve the repeat count */
    n = 1;
  if (n < 1)			/* search backwards */
    return (backsearch(f, -n));

  /* ask the user for the text of a pattern */
  if ((status = readpattern("Search")) != TRUE)
    return (status);

  /* search for the pattern */
  while (n-- > 0)
    {
      if ((status = forscan(&pat[0], PTEND)) == FALSE)
	break;
    }

  /* and complain if not there */
  if (status == FALSE)
    mlwrite("Not found");
  return (status);
}

int forwhunt(int f, int n)
{
  int status = 0;

  /* resolve the repeat count */
  if (n == 0)
    n = 1;
  if (n < 1)			/* search backwards */
    return (backhunt(f, -n));

  /* Make sure a pattern exists */
  if (pat[0] == 0)
    {
      mlwrite("No pattern set");
      return (FALSE);
    }
  /* search for the pattern */
  while (n-- > 0)
    {
      if ((status = forscan(&pat[0], PTEND)) == FALSE)
	break;
    }

  /* and complain if not there */
  if (status == FALSE)
    mlwrite("Not found");
  return (status);
}

/*
 * Reverse search. Get a search string from the user, and search, starting at
 * "." and proceeding toward the front of the buffer. If found "." is left
 * pointing at the first character of the pattern [the last character that was
 * matched]. Bound to "C-R"
 */
int backsearch(int f, int n)
{
  int s;

  if (n == 0)			/* resolve null and negative arguments */
    n = 1;
  if (n < 1)
    return (forwsearch(f, -n));

  if ((s = readpattern("Reverse search")) != TRUE)	/* get a pattern to search */
    return (s);

  return bsearch(f, n);		/* and go search for it */
}

/* hunt backward for the last search string entered
 */
int backhunt(int f, int n)
{
  if (n == 0)			/* resolve null and negative arguments */
    n = 1;
  if (n < 1)
    return (forwhunt(f, -n));

  if (pat[0] == 0)		/* Make sure a pattern exists */
    {
      mlwrite("No pattern set");
      return (FALSE);
    }
  return bsearch(f, n);		/* go search */
}

int bsearch(int f, int n)
{
  LINE *clp, *tlp;
  char *epp, *pp;
  int cbo, tbo, c;

  /* find a pointer to the end of the pattern */
  for (epp = &pat[0]; epp[1] != 0; ++epp)
    ;

  /* make local copies of the starting location */
  clp = curwp->w_dotp;
  cbo = curwp->w_doto;

  while (n-- > 0)
    {
      for (;;)
	{
	  /* if we are at the begining of the line, wrap back around */
	  if (cbo == 0)
	    {
	      clp = lback(clp);

	      if (clp == curbp->b_linep)
		{
		  mlwrite("Not found");
		  return (FALSE);
		}
	      cbo = llength(clp) + 1;
	    }
	  /* fake the <NL> at the end of a line */
	  if (--cbo == llength(clp))
	    c = '\n';
	  else
	    c = lgetc(clp, cbo);

	  /* check for a match against the end of the pattern */
	  if (eq(c, *epp) != FALSE)
	    {
	      tlp = clp;
	      tbo = cbo;
	      pp = epp;
	      /* scanning backwards through the rest of the pattern
	       * looking for a match */
	      while (pp != &pat[0])
		{
		  /* wrap across a line break */
		  if (tbo == 0)
		    {
		      tlp = lback(tlp);
		      if (tlp == curbp->b_linep)
			goto fail;

		      tbo = llength(tlp) + 1;
		    }
		  /* fake the <NL> */
		  if (--tbo == llength(tlp))
		    c = '\n';
		  else
		    c = lgetc(tlp, tbo);

		  if (eq(c, *--pp) == FALSE)
		    goto fail;
		}

	      /* A Match!  reset the current cursor */
	      curwp->w_dotp = tlp;
	      curwp->w_doto = tbo;
	      curwp->w_flag |= WFMOVE;
	      goto next;
	    }
	fail:;
	}
    next:;
    }
  return (TRUE);
}

/*
 * Compare two characters. The "bc" comes from the buffer. It has it's case
 * folded out. The "pc" is from the pattern
 */
int eq(int bc, int pc)
{
  if (bc >= 'a' && bc <= 'z')
    bc -= 0x20;
  if (pc >= 'a' && pc <= 'z')
    pc -= 0x20;
  if (bc == pc)
    return (TRUE);
  return (FALSE);
}

/*
 * Read a pattern. Stash it in the external variable "pat". The "pat" is not
 * updated if the user types in an empty line. If the user typed an empty
 * line, and there is no old pattern, it is an error. Display the old pattern,
 * in the style of Jeff Lomicka. There is some do-it-yourself control
 * expansion.
 */
int readpattern(char *prompt)
{
  char tpat[NPAT + 20];
  int s;

  strncpy(tpat, prompt, NPAT-12); /* copy prompt to output string */
  strncat(tpat, " [", 3);	/* build new prompt string */
  expandp(&pat[0], &tpat[strlen (tpat)], NPAT / 2); /* add old pattern */
  strncat(tpat, "]: ", 4);

  s = mlreplyt(tpat, tpat, NPAT, 10); /* Read pattern */

  if (s == TRUE)		/* Specified */
    strncpy(pat, tpat, NPAT);
  else if (s == FALSE && pat[0] != 0) /* CR, but old one */
    s = TRUE;

  return (s);
}

/*
 * Search and replace (ESC-R)
 */
int sreplace(int f, int n)
{
  return (replaces(FALSE, f, n));
}

/*
 * search and replace with query (ESC-CTRL-R)
 */
int qreplace(int f, int n)
{
  return (replaces(TRUE, f, n));
}

/*
 * replaces: search for a string and replace it with another string. query
 * might be enabled (according to kind)
 */
int replaces(int kind, int f, int n)
{
  LINE *origline;		/* original "." position */
  char tmpc;			/* temporary character */
  char c;			/* input char for query */
  char tpat[NPAT];		/* temporary to hold search pattern */
  int i;			/* loop index */
  int s;			/* success flag on pattern inputs */
  int slength, rlength;		/* length of search and replace strings */
  int numsub;			/* number of substitutions */
  int nummatch;			/* number of found matches */
  int nlflag;			/* last char of search string a <NL>? */
  int nlrepl;			/* was a replace done on the last line? */
  int origoff;			/* and offset (for . query option) */

  /* check for negative repititions */
  if (f && n < 0)
    return (FALSE);

  /* ask the user for the text of a pattern */
  if ((s = readpattern((kind == FALSE ? "Replace" : "Query replace"))) != TRUE)
    return (s);
  strncpy(&tpat[0], &pat[0], NPAT); /* salt it away */

  /* ask for the replacement string */
  strncpy(&pat[0], &rpat[0], NPAT); /* set up default string */
  if ((s = readpattern("with")) == ABORT)
    return (s);

  /* move everything to the right place and length them */
  strncpy(&rpat[0], &pat[0], NPAT);
  strncpy(&pat[0], &tpat[0], NPAT);
  slength = strlen(&pat[0]);
  rlength = strlen(&rpat[0]);

  /* set up flags so we can make sure not to do a recursive replace on the
   * last line */
  nlflag = (pat[slength - 1] == '\n');
  nlrepl = FALSE;

  /* build query replace question string */
  strncpy(tpat, "Replace '", 10);
  expandp(&pat[0], &tpat[strlen (tpat)], NPAT / 3);
  strncat(tpat, "' with '", 9);

  expandp(&rpat[0], &tpat[strlen (tpat)], NPAT / 3);
  strncat(tpat, "'? ", 4);

  /* save original . position */
  origline = curwp->w_dotp;
  origoff = curwp->w_doto;

  /* scan through the file */
  numsub = 0;
  nummatch = 0;
  while ((f == FALSE || n > nummatch) &&
	 (nlflag == FALSE || nlrepl == FALSE))
    {
      /* search for the pattern */
      if (forscan(&pat[0], PTBEG) != TRUE)
	break;			/* all done */
      ++nummatch;		/* increment # of matches */

      /* check if we are on the last line */
      nlrepl = (lforw (curwp->w_dotp) == curwp->w_bufp->b_linep);

      /* check for query */
      if (kind)
	{
	  /* get the query */
	  mlwrite(&tpat[0], &pat[0], &rpat[0]);
	qprompt:
	  update();		/* show the proposed place to change */
	  c = (*term.t_getchar) (); /* and input */
	  mlwrite("");		/* and clear it */

	  /* and respond appropriately */
	  switch (c)
	    {
	    case 'y':		/* yes, substitute */
	    case ' ':
	      break;

	    case 'n':		/* no, onword */
	      forwchar(FALSE, 1);
	      continue;

	    case '!':		/* yes/stop asking */
	      kind = FALSE;
	      break;

	    case '.':		/* abort! and return */
	      /* restore old position */
	      curwp->w_dotp = origline;
	      curwp->w_doto = origoff;
	      curwp->w_flag |= WFMOVE;

	    case BELL:		/* abort! and stay */
	      mlwrite("Aborted!");
	      return (FALSE);

	    case 0x0d:		/* controlled exit */
	    case 'q':
	      return (TRUE);

	    default:		/* bitch and beep */
	      (*term.t_beep) ();

	    case '?':		/* help me */
	      mlwrite("(Y)es, (N)o, (!)Do the rest, (^G,RET,q)Abort, (.)Abort back, (?)Help: ");
	      goto qprompt;
	    }
	}
      /* delete the sucker */
      if (ldelete(slength, FALSE) != TRUE)
	{
	  /* error while deleting */
	  mlwrite("ERROR while deleting");
	  return (FALSE);
	}
      /* and insert its replacement */
      for (i = 0; i < rlength; i++)
	{
	  tmpc = rpat[i];
	  s = (tmpc == '\n' ? lnewline() : linsert(1, tmpc));
	  if (s != TRUE)
	    {
	      /* error while inserting */
	      mlwrite("Out of memory while inserting");
	      return (FALSE);
	    }
	}

      numsub++;			/* increment # of substitutions */
    }

  /* and report the results */
  mlwrite("%d substitutions", numsub);
  return (TRUE);
}

/* search forward for a <patrn>
 */
int forscan(char *patrn, int leavep)
{
  LINE *curline;		/* current line during scan */
  LINE *lastline;		/* last line position during scan */
  LINE *matchline;		/* current line during matching */
  char *patptr;			/* pointer into pattern */
  int curoff;			/* position within current line */
  int lastoff;			/* position within last line */
  int c;			/* character at current position */
  int matchoff;			/* position in matching line */

  /* setup local scan pointers to global "." */
  curline = curwp->w_dotp;
  curoff = curwp->w_doto;

  /* scan each character until we hit the head link record */
  while (curline != curbp->b_linep)
    {
      /* save the current position in case we need to restore it on a match */
      lastline = curline;
      lastoff = curoff;

      /* get the current character resolving EOLs */
      if (curoff == llength(curline))
	{			/* if at EOL */
	  curline = lforw(curline); /* skip to next line */
	  curoff = 0;
	  c = '\n';		/* and return a <NL> */
	}
      else
	c = lgetc(curline, curoff++); /* get the char */

      /* test it against first char in pattern */
      if (eq(c, patrn[0]) != FALSE) /* if we find it. */
	{
	  /* setup match pointers */
	  matchline = curline;
	  matchoff = curoff;
	  patptr = &patrn[0];

	  /* scan through patrn for a match */
	  while (*++patptr != 0)
	    {
	      /* advance all the pointers */
	      if (matchoff == llength(matchline))
		{
		  /* advance past EOL */
		  matchline = lforw(matchline);
		  matchoff = 0;
		  c = '\n';
		}
	      else
		c = lgetc(matchline, matchoff++);

	      /* and test it against the pattern */
	      if (eq(*patptr, c) == FALSE)
		goto fail;
	    }

	  /* A SUCCESSFULL MATCH!!! */
	  /* reset the global "." pointers */
	  if (leavep == PTEND)
	    {			/* at end of string */
	      curwp->w_dotp = matchline;
	      curwp->w_doto = matchoff;
	    }
	  else
	    {			/* at begining of string */
	      curwp->w_dotp = lastline;
	      curwp->w_doto = lastoff;
	    }
	  curwp->w_flag |= WFMOVE; /* flag that we have moved */
	  return (TRUE);
	}
    fail:;			/* continue to search */
    }
  /* we could not find a match */
  return (FALSE);
}

/* expandp: expand control key sequences for output
 */
void expandp(char *srcstr, char *deststr, int maxlength)
{
  char c;			/* current char to translate */

  /* scan through the string */
  while ((c = *srcstr++) != 0)
    {
      if (c < 0x20 || c == 0x7f)
	{			/* control character */
	  *deststr++ = '^';
	  *deststr++ = c ^ 0x40;
	  maxlength -= 2;
	}
      else if (c == '%')
	{
	  *deststr++ = '%';
	  *deststr++ = '%';
	  maxlength -= 2;
	}
      else
	{			/* any other character */
	  *deststr++ = c;
	  maxlength--;
	}

      /* check for maxlength */
      if (maxlength < 4)
	{
	  *deststr++ = '$';
	  *deststr = '\0';
	  return;
	}
    }
  *deststr = '\0';
  return;
}
