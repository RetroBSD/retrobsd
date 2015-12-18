/* This file is in the public domain. */

/*
 * The routines in this file read and write ASCII files from the disk. All of
 * the knowledge about files is here. A better message writing scheme should
 * be used
 */

#include <stdio.h>		/* fopen(3), et.al. */
#include "estruct.h"

extern void mlwrite();

int ffropen(char *);
int ffwopen(char *);
int ffclose();
int ffputline(char [], int);
int ffgetline(char [], int);

FILE *ffp;			/* File pointer, all functions */

/*
 * Open a file for reading.
 */
int ffropen(char *fn)
{
  if ((ffp = fopen(fn, "r")) == NULL)
    return (FIOFNF);
  return (FIOSUC);
}

/*
 * Open a file for writing. Return TRUE if all is well, and FALSE on error
 * (cannot create).
 */
int ffwopen(char *fn)
{
  if ((ffp = fopen(fn, "w")) == NULL)
    {
      mlwrite("Cannot open file for writing");
      return (FIOERR);
    }
  return (FIOSUC);
}

/*
 * Close a file. Should look at the status in all systems.
 */
int ffclose()
{
  if (fclose(ffp) != FALSE)
    {
      mlwrite("Error closing file");
      return (FIOERR);
    }
  return (FIOSUC);
}

/*
 * Write a line to the already opened file. The "buf" points to the buffer,
 * and the "nbuf" is its length, less the free newline. Return the status.
 * Check only at the newline.
 */
int ffputline(char buf[], int nbuf)
{
  int i;

  for (i = 0; i < nbuf; ++i)
    fputc(buf[i] & 0xFF, ffp);

  fputc('\n', ffp);

  if (ferror(ffp))
    {
      mlwrite("Write I/O error");
      return (FIOERR);
    }
  return (FIOSUC);
}

/*
 * Read a line from a file, and store the bytes in the supplied buffer. The
 * "nbuf" is the length of the buffer. Complain about long lines and lines at
 * the end of the file that don't have a newline present. Check for I/O errors
 * too. Return status.
 */
int ffgetline(char buf[], int nbuf)
{
  int c, i;

  i = 0;

  while ((c = fgetc(ffp)) != EOF && c != '\n')
    {
      if (i >= nbuf - 2)
	{
	  buf[nbuf - 2] = c;	   /* store last char read */
	  buf[nbuf - 1] = 0;	   /* and terminate it */
	  mlwrite("File has long lines");
	  return (FIOLNG);
	}
      buf[i++] = c;
    }

  if (c == EOF)
    {
      if (ferror(ffp))
	{
	  mlwrite("File read error");
	  return (FIOERR);
	}
      if (i != 0)
	{
	  mlwrite("No newline at EOF");
	  return (FIOERR);
	}
      return (FIOEOF);
    }
  buf[i] = 0;
  return (FIOSUC);
}
