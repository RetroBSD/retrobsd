/* This file is in the public domain. */

/* termios video driver */

#define termdef 1	/* don't define "term" externally */

/* Did you remember to set FORCE_COLS? */
#ifndef FORCE_COLS
#define FORCE_COLS 80
#endif

/* Did you remember to set FORCE_ROWS? */
#ifndef FORCE_ROWS
#define FORCE_ROWS 24
#endif

/*
 * XXX:
 * Default/sane(?) maximum column and row sizes.
 * Taken from mg1a.
 *
 * Let the user override this with a
 * CFLAGS += -DMAXCOL=XXX -DMAXROW=XXX
 * line in the Makefile.
 */
#ifndef MAXCOL
#define MAXCOL 132
#endif

#ifndef MAXROW
#define MAXROW 66
#endif

#include <stdio.h>	/* puts(3), snprintf(3) */
#include "estruct.h"
#include "edef.h"
#undef CTRL		/* Needs to be done here. */
#include <sys/ioctl.h>

extern int tgetent();
extern char *tgetstr();
extern char *tgoto();
extern void tputs();

extern char *getenv();
extern void ttopen();
extern int ttgetc();
extern void ttputc();
extern void ttflush();
extern void ttclose();

extern void panic();

void getwinsize();
void tcapopen();
void tcapmove(int row, int col);
void tcapeeol();
void tcapeeop();
void tcaprev();
void tcapbeep();

#define	MARGIN 8
#define	SCRSIZ 64
#define BEL 0x07
#define TCAPSLEN 64

char tcapbuf[TCAPSLEN];		/* capabilities actually used */
char *CM, *CE, *CL, *SO, *SE;

TERM term = {
  0, 0, MARGIN, SCRSIZ, tcapopen, ttclose, ttgetc, ttputc,
  ttflush, tcapmove, tcapeeol, tcapeeop, tcapbeep, tcaprev
};

void getwinsize()
{
  int cols = FORCE_COLS;
  int rows = FORCE_ROWS;

  /* Too small and we're out */
  if ((cols < 10) || (rows < 3))
      panic("Too few columns or rows");

  if (FORCE_COLS > MAXCOL)
      cols = MAXCOL;
  if (FORCE_ROWS > MAXROW)
      rows = MAXROW;

  term.t_ncol = cols;
  term.t_nrow = rows - 1;
}

void tcapopen()
{
  char tcbuf[1024];
  char *p, *tv_stype;

  if ((tv_stype = getenv("TERM")) == NULL)
      panic("TERM not defined");
  if ((tgetent(tcbuf, tv_stype)) != 1)
      panic("Unknown terminal type");
  p = tcapbuf;
  CL = tgetstr("cl", &p);
  CM = tgetstr("cm", &p);
  CE = tgetstr("ce", &p);
  SE = tgetstr("se", &p);
  SO = tgetstr("so", &p);

  if (CE == NULL)
    eolexist = FALSE;
  if (SO != NULL && SE != NULL)
    revexist = TRUE;
  if (CL == NULL || CM == NULL)
      panic("Need cl & cm abilities");
  if (p >= &tcapbuf[TCAPSLEN])	/* XXX */
      panic("Description too big");
  ttopen ();
}

void tcaprev(int state)
{
  if (revexist)
    tputs((state ? SO : SE), 1, ttputc);
}

void tcapmove (int row, int col)
{
  tputs(tgoto(CM, col, row), 1, ttputc);
}

void tcapeeol()
{
  tputs(CE, 1, ttputc);
}

void tcapeeop()
{
  tputs(CL, 1, ttputc);
}

void tcapbeep()
{
  ttputc(BEL);
}
