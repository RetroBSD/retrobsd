/* This file is in the public domain. */

/* termios video driver */

#include <stdio.h>  /* puts(3), snprintf(3) */
#include <stdlib.h> /* getenv() */

#include "edef.h"
#undef CTRL /* Needs to be done here. */
#include <sys/ioctl.h>
#include <term.h>

#define MARGIN 8
#define SCRSIZ 64
#define BEL 0x07
#define TCAPSLEN 64

char tcapbuf[TCAPSLEN]; /* capabilities actually used */
char *CM, *CE, *CL, *SO, *SE;

char *UP; /* required by libtermcap */
char *BC;
char PC = '\0';
short ospeed = 0;

TERM term = { 0,      0,       MARGIN,   SCRSIZ,   tcapopen, ttclose,  ttgetc,
              ttputc, ttflush, tcapmove, tcapeeol, tcapeeop, tcapbeep, tcaprev };

void getwinsize(void)
{
    struct winsize ws;

    if (ioctl(0, TIOCGWINSZ, &ws) == 0) {
        term.t_ncol = ws.ws_col;
        rows = ws.ws_row;
    }

    /* Too small and we hard code */
    if ((term.t_ncol < 10) || (rows < 3)) {
        term.t_ncol = 80;
        rows = 24;
    }

    term.t_nrow = rows - 1;
}

void tcapopen(void)
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
    if (p >= &tcapbuf[TCAPSLEN]) /* XXX */
        panic("Description too big");
    ttopen();
}

void tcaprev(int state)
{
    if (revexist)
        tputs((state ? SO : SE), 1, ttputc);
}

void tcapmove(int row, int col)
{
    tputs(tgoto(CM, col, row), 1, ttputc);
}

void tcapeeol(void)
{
    tputs(CE, 1, ttputc);
}

void tcapeeop(void)
{
    tputs(CL, 1, ttputc);
}

void tcapbeep(void)
{
    ttputc(BEL);
}
