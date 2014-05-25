/* This file is in the public domain. */

/*
 * The routines in this file handle the reading and writing of disk files.
 * All details about the reading and writing of the disk are in "fileio.c"
 */

#include <string.h>		/* strncpy(3) */
#include "estruct.h"
#include "edef.h"

extern int mlreply(char *prompt, char *buf, int nbuf);
extern int swbuffer(BUFFER *bp);
extern void mlwrite();
extern int bclear(BUFFER *bp);
extern int ffropen(char *fn);
extern int ffgetline(char buf[], int nbuf);
extern int ffwopen(char *fn);
extern int ffclose();
extern int ffputline(char buf[], int nbuf);
extern BUFFER *bfind();
extern LINE *lalloc();

int fileread(int f, int n);
int insfile(int f, int n);
int filefind(int f, int n);
int getfile(char fname[]);
int readin(char fname[]);
void makename(char bname[], char fname[]);
int filewrite(int f, int n);
int filesave(int f, int n);
int writeout(char *fn);
int filename(int f, int n);
int ifile(char fname[]);

/*
 * Read a file into the current buffer. This is really easy; all you do it
 * find the name of the file, and call the standard "read a file into the
 * current buffer" code. Bound to "C-X C-R"
 */
int fileread(int f, int n)
{
  int s;
  char fname[NFILEN];

  if ((s = mlreply("Read file: ", fname, NFILEN)) != TRUE)
    return (s);
  return (readin(fname));
}

/*
 * Insert a file into the current buffer. This is really easy; all you do it
 * find the name of the file, and call the standard "insert a file into the
 * current buffer" code. Bound to "C-X C-I".
 */
int insfile(int f, int n)
{
  int s;
  char fname[NFILEN];

  if ((s = mlreply("Insert file: ", fname, NFILEN)) != TRUE)
    return (s);
  return (ifile(fname));
}

/*
 * Select a file for editing. Look around to see if you can find the fine in
 * another buffer; if you can find it just switch to the buffer. If you cannot
 * find the file, create a new buffer, read in the text, and switch to the new
 * buffer. Bound to C-X C-F.
 */
int filefind(int f, int n)
{
  char fname[NFILEN];		/* file user wishes to find */
  int s;			/* status return */

  if ((s = mlreply("Find file: ", fname, NFILEN)) != TRUE)
    return (s);
  return (getfile(fname));
}

int getfile(char fname[])
{
  BUFFER *bp;
  LINE *lp;
  char bname[NBUFN];		/* buffer name to put file */
  int i, s;

  for (bp = bheadp; bp != (BUFFER*)0; bp = bp->b_bufp)
    {
      if ((bp->b_flag & BFTEMP) == 0 && strcmp(bp->b_fname, fname) == 0)
	{
	  if (--curbp->b_nwnd == 0)
	    {
	      curbp->b_dotp = curwp->w_dotp;
	      curbp->b_doto = curwp->w_doto;
	      curbp->b_markp = curwp->w_markp;
	      curbp->b_marko = curwp->w_marko;
	    }
	  swbuffer(bp);
	  lp = curwp->w_dotp;
	  i = curwp->w_ntrows / 2;
	  while (i-- && lback(lp) != curbp->b_linep)
	    lp = lback(lp);
	  curwp->w_linep = lp;
	  curwp->w_flag |= WFMODE | WFHARD;
	  mlwrite("[Old buffer]");
	  return (TRUE);
	}
    }
  makename(bname, fname);	/* New buffer name */
  while ((bp = bfind(bname, FALSE, 0)) != (BUFFER*)0)
    {
      s = mlreply("Buffer name: ", bname, NBUFN);
      if (s == ABORT)		/* ^G to just quit */
	return (s);
      if (s == FALSE)
	{			/* CR to clobber it */
	  makename(bname, fname);
	  break;
	}
    }
  if (bp == (BUFFER*)0 && (bp = bfind(bname, TRUE, 0)) == (BUFFER*)0)
    {
      mlwrite("Cannot create buffer");
      return (FALSE);
    }
  if (--curbp->b_nwnd == 0)
    {				/* Undisplay */
      curbp->b_dotp = curwp->w_dotp;
      curbp->b_doto = curwp->w_doto;
      curbp->b_markp = curwp->w_markp;
      curbp->b_marko = curwp->w_marko;
    }
  curbp = bp;			/* Switch to it */
  curwp->w_bufp = bp;
  curbp->b_nwnd++;
  return (readin(fname));	/* Read it in */
}

/*
 * Read file "fname" into the current buffer, blowing away any text found
 * there. Called by both the read and find commands. Return the final status
 * of the read. Also called by the mainline, to read in a file specified on
 * the command line as an argument.
 */
int readin(char fname[])
{
  LINE *lp1, *lp2;
  WINDOW *wp;
  BUFFER *bp;
  char line[NLINE];
  int nbytes, s, i;
  int nline = 0;		/* initialize here to silence a gcc warning */
  int lflag;			/* any lines longer than allowed? */

  bp = curbp;			/* Cheap */
  if ((s = bclear(bp)) != TRUE) /* Might be old */
    return (s);
  bp->b_flag &= ~(BFTEMP | BFCHG);
  strncpy(bp->b_fname, fname, NFILEN);
  if ((s = ffropen(fname)) == FIOERR) /* Hard file open */
    goto out;
  if (s == FIOFNF)
    {				/* File not found */
      mlwrite("[New file]");
      goto out;
    }
  mlwrite("[Reading file]");
  lflag = FALSE;
  while ((s = ffgetline(line, NLINE)) == FIOSUC || s == FIOLNG)
    {
      if (s == FIOLNG)
	lflag = TRUE;
      nbytes = strlen(line);
      if ((lp1 = lalloc(nbytes)) == NULL)
	{
	  s = FIOERR;		/* Keep message on the display */
	  break;
	}
      lp2 = lback(curbp->b_linep);
      lp2->l_fp = lp1;
      lp1->l_fp = curbp->b_linep;
      lp1->l_bp = lp2;
      curbp->b_linep->l_bp = lp1;
      for (i = 0; i < nbytes; ++i)
	lputc(lp1, i, line[i]);
      ++nline;
    }
  ffclose();			/* Ignore errors */
  if (s == FIOEOF)
    {				/* Don't zap message! */
      if (nline != 1)
	mlwrite("[Read %d lines]", nline);
      else
	mlwrite("[Read 1 line]");
    }
  if (lflag)
    {
      if (nline != 1)
	mlwrite("[Read %d lines: Long lines wrapped]", nline);
      else
	mlwrite("[Read 1 line: Long lines wrapped]");
    }
  curwp->w_bufp->b_lines = nline;
 out:
  for (wp = wheadp; wp != NULL; wp = wp->w_wndp)
    {
      if (wp->w_bufp == curbp)
	{
	  wp->w_linep = lforw(curbp->b_linep);
	  wp->w_dotp = lforw(curbp->b_linep);
	  wp->w_doto = 0;
	  wp->w_markp = NULL;
	  wp->w_marko = 0;
	  wp->w_flag |= WFMODE | WFHARD;
	}
    }
  if (s == FIOERR || s == FIOFNF) /* False if error */
    return (FALSE);
  return (TRUE);
}

/*
 * Take a file name, and from it fabricate a buffer name. This routine knows
 * about the syntax of file names on the target system. I suppose that this
 * information could be put in a better place than a line of code.
 */
void makename(char bname[], char fname[])
{
  char *cp1, *cp2;

  cp1 = &fname[0];
  while (*cp1 != 0)
    ++cp1;

  while (cp1 != &fname[0] && cp1[-1] != '/')
    --cp1;
  cp2 = &bname[0];
  while (cp2 != &bname[NBUFN - 1] && *cp1 != 0 && *cp1 != ';')
    *cp2++ = *cp1++;
  *cp2 = 0;
}

/*
 * Ask for a file name, and write the contents of the current buffer to that
 * file. Update the remembered file name and clear the buffer changed flag.
 * This handling of file names is different from the earlier versions, and is
 * more compatable with Gosling EMACS than with ITS EMACS. Bound to "C-X C-W".
 */
int filewrite(int f, int n)
{
  WINDOW *wp;
  char fname[NFILEN];
  int s;

  if ((s = mlreply("Write file: ", fname, NFILEN)) != TRUE)
    return (s);
  if ((s = writeout(fname)) == TRUE)
    {
      strncpy(curbp->b_fname, fname, NFILEN);
      curbp->b_flag &= ~BFCHG;
      wp = wheadp;		/* Update mode lines */
      while (wp != NULL)
	{
	  if (wp->w_bufp == curbp)
	    wp->w_flag |= WFMODE;
	  wp = wp->w_wndp;
	}
    }
  return (s);
}

/*
 * Save the contents of the current buffer in its associatd file. No nothing
 * if nothing has changed (this may be a bug, not a feature). Error if there
 * is no remembered file name for the buffer. Bound to "C-X C-S". May get
 * called by "C-Z"
 */
int filesave(int f, int n)
{
  WINDOW *wp;
  int s;

  if ((curbp->b_flag & BFCHG) == 0) /* Return, no changes */
    return (TRUE);
  if (curbp->b_fname[0] == 0)
    {				/* Must have a name */
      mlwrite("No file name");
      return (FALSE);
    }
  if ((s = writeout(curbp->b_fname)) == TRUE)
    {
      curbp->b_flag &= ~BFCHG;
      wp = wheadp;		/* Update mode lines */
      while (wp != NULL)
	{
	  if (wp->w_bufp == curbp)
	    wp->w_flag |= WFMODE;
	  wp = wp->w_wndp;
	}
    }
  return (s);
}

/*
 * This function performs the details of file writing. Uses the file
 * management routines in the "fileio.c" package. The number of lines written
 * is displayed. Sadly, it looks inside a LINE; provide a macro for this. Most
 * of the grief is error checking of some sort.
 */
int writeout(char *fn)
{
  LINE *lp;
  int nline, s;

  if ((s = ffwopen(fn)) != FIOSUC) /* Open writes message */
    return (FALSE);
  mlwrite("[Writing]");		/* tell us were writing */
  lp = lforw(curbp->b_linep);	/* First line */
  nline = 0;			/* Number of lines */
  while (lp != curbp->b_linep)
    {
      if ((s = ffputline(&lp->l_text[0], llength(lp))) != FIOSUC)
	break;
      ++nline;
      lp = lforw(lp);
    }
  if (s == FIOSUC)
    {				/* No write error */
      s = ffclose();
      if (s == FIOSUC)
	{			/* No close error */
	  if (nline != 1)
	    mlwrite("[Wrote %d lines]", nline);
	  else
	    mlwrite("[Wrote 1 line]");
	}
    }
  else				/* ignore close error */
    ffclose();			/* if a write error */
  if (s != FIOSUC)		/* some sort of error */
    return (FALSE);
  return (TRUE);
}

/*
 * The command allows the user to modify the file name associated with the
 * current buffer. It is like the "f" command in UNIX "ed". The operation is
 * simple; just zap the name in the BUFFER structure, and mark the windows as
 * needing an update. You can type a blank line at the prompt if you wish.
 */
int filename(int f, int n)
{
  WINDOW *wp;
  char fname[NFILEN];
  int s;

  if ((s = mlreply("Name: ", fname, NFILEN)) == ABORT)
    return (s);
  if (s == FALSE)
    strncpy(curbp->b_fname, "", 1);
  else
    strncpy(curbp->b_fname, fname, NFILEN);
  wp = wheadp;			/* update mode lines */
  while (wp != NULL)
    {
      if (wp->w_bufp == curbp)
	wp->w_flag |= WFMODE;
      wp = wp->w_wndp;
    }
  return (TRUE);
}

/*
 * Insert file "fname" into the current buffer, Called by insert file command.
 * Return the final status of the read.
 */
int ifile(char fname[])
{
  LINE *lp0, *lp1, *lp2;
  BUFFER *bp;
  char line[NLINE];
  int i, s, nbytes;
  int nline = 0;
  int lflag;			/* any lines longer than allowed? */

  bp = curbp;			/* Cheap */
  bp->b_flag |= BFCHG;		/* we have changed */
  bp->b_flag &= ~BFTEMP;	/* and are not temporary */
  if ((s = ffropen(fname)) == FIOERR) /* Hard file open */
    goto out;
  if (s == FIOFNF)
    {				/* File not found */
      mlwrite("[No such file]");
      return (FALSE);
    }
  mlwrite("[Inserting file]");

  /* back up a line and save the mark here */
  curwp->w_dotp = lback(curwp->w_dotp);
  curwp->w_doto = 0;
  curwp->w_markp = curwp->w_dotp;
  curwp->w_marko = 0;

  lflag = FALSE;
  while ((s = ffgetline(line, NLINE)) == FIOSUC || s == FIOLNG)
    {
      if (s == FIOLNG)
	lflag = TRUE;
      nbytes = strlen(line);
      if ((lp1 = lalloc(nbytes)) == NULL)
	{
	  s = FIOERR;		/* keep message on the */
	  break;		/* display */
	}
      lp0 = curwp->w_dotp;	/* line previous to insert */
      lp2 = lp0->l_fp;		/* line after insert */

      /* re-link new line between lp0 and lp2 */
      lp2->l_bp = lp1;
      lp0->l_fp = lp1;
      lp1->l_bp = lp0;
      lp1->l_fp = lp2;

      /* and advance and write out the current line */
      curwp->w_dotp = lp1;
      for (i = 0; i < nbytes; ++i)
	lputc(lp1, i, line[i]);
      ++nline;
    }
  ffclose();			/* Ignore errors */
  curwp->w_markp = lforw(curwp->w_markp);
  if (s == FIOEOF)
    {				/* Don't zap message! */
      if (nline != 1)
	mlwrite("[Inserted %d lines]", nline);
      else
	mlwrite("[Inserted 1 line]");
    }
  if (lflag)
    {
      if (nline != 1)
	mlwrite("[Inserted %d lines: Long lines wrapped]", nline);
      else
	mlwrite("[Inserted 1 line: Long lines wrapped]");
    }
 out:
  /* advance to the next line and mark the window for changes */
  curwp->w_dotp = lforw(curwp->w_dotp);
  curwp->w_flag |= WFHARD;

  /* copy window parameters back to the buffer structure */
  curbp->b_dotp = curwp->w_dotp;
  curbp->b_doto = curwp->w_doto;
  curbp->b_markp = curwp->w_markp;
  curbp->b_marko = curwp->w_marko;

  /* we need to update number of lines in the buffer */
  curwp->w_bufp->b_lines += nline;

  if (s == FIOERR)		/* False if error */
    return (FALSE);
  return (TRUE);
}
