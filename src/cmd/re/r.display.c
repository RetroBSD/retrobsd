/*
 * Interface to display - logical level.
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include "r.defs.h"

/*
 * Draw lines from lo to lf.
 * In case lo is negative:
 * - draw only line lf;
 * - use cline as a source;
 * - draw only from column -lo.
 */
void drawlines(lo, lf)
    int lo, lf;
{
    register int i, l0;
    int j, k, l1;
    char lmc, *cp, draw_border;

    l0 = lo;
    lo += 2;
    if (lo > 0)
        lo = 0;         /* Initial column */
    l1 = lo;
    lmc = (curwin->base_col == curwin->text_col ? 0 :
        curwksp->offset == 0 ? LMCH : MLMCH);
    draw_border = (curwin->text_col + curwin->text_maxcol < curwin->max_col);
    while (l0 <= lf) {
        lo = l1;
        if (l0 < 0) {
            l0 = lf;
            lf = -1;
            i = 0;
        } else {
            if (l0 != lf && interrupt())
                return;
            i = wksp_seek(curwksp, curwksp->topline + l0);
            if (i && lmc != 0)
                lmc = ELMCH;
        }
        if (! lmc || lo < 0 || lmc == curwin->leftbar[l0])
            poscursor(0, l0);
        else {
            poscursor(-1, l0);
            wputc(lmc, 0);
        }
        curwin->leftbar[l0] = lmc;
        if (draw_border != 0)
            draw_border = RMCH;
        if (i != 0)
            i = 0;
        else {
            if (lf >= 0)
                cline_read(1);
            i = (cline_len - 1) - curwksp->offset;
            if (i < 0)
                i = 0;
            else if (i > curwin->text_maxcol) {
                if (draw_border && i > 1 + curwin->text_maxcol)
                    draw_border = MRMCH;
                i = 1 + curwin->text_maxcol;
            }
        }

        /*
         * Draw symbols.
         * Skip initial spaces, when possible.
         */
        if (lo == 0) {
            int fc;
            for (fc=0; cline != 0 && cline[curwksp->offset + fc]==' '; fc++);
            j = curwin->text_maxcol + 1;
            if (fc > j)
                fc = j;
            if (fc > 255)
                fc = 255;
            lo = (curwin->firstcol[l0] > fc) ? - fc : - curwin->firstcol[l0];
            if (i + lo <= 0)
                lo = 0;
            else
                curwin->firstcol[l0] = fc;
        }
        if (lo)
            poscursor(-lo, l0);
        j = i + lo;
        cp = cline + curwksp->offset - lo;
        while(j--)
            putch(*cp++);
        cursorcol += (i + lo);
        if (curwin->lastcol[l0] < cursorcol)
            curwin->lastcol[l0] = cursorcol;

        /* Fill a tail by spaces. */
        j = curwin->lastcol[l0];
        k = j - i;
        if (k > 0) {
            putblanks(k);
        }
        if (i > curwin->text_maxcol) {
            /* Too long line - add continuation mark. */
            pcursor(curwin->text_col + curwin->text_maxcol, l0);
            putch('~');
        }
        if (curwin->text_col + cursorcol >= NCOLS) {
            /* Cursor lost after last column: move it to known position. */
            cursorcol = 0;
            pcursor(0, l0);
        }
        if (draw_border && draw_border != curwin->rightbar[l0]) {
            poscursor(curwin->max_col - curwin->text_col, l0);
            wputc(draw_border, 0);
        }
        curwin->rightbar[l0] = draw_border;
        curwin->lastcol[l0] = (k > 0 ? i : j);
        ++l0;
    }
}

/*
 * Position a cursor in a current window.
 */
void poscursor(col, lin)
    int col, lin;
{
    register int scol;
    int slin;

    if (cursorline == lin) {
        if (cursorcol == col)
            return;
        if ((cursorcol == col-1) && putch(CORT)) {
            ++cursorcol;
            return;
        }
        if ((cursorcol == col+1) && putch(COLT)) {
            --cursorcol;
            return;
        }
    }
    if (cursorcol == col) {
        if ((cursorline == lin-1) && putch(CODN)) {
            ++cursorline;
            return;
        }
        if ((cursorline == lin+1) && putch(COUP)) {
            --cursorline;
            return;
        }
    }
    scol = col + curwin->text_col;
    slin = lin + curwin->text_row;    /* screen col, lin */
    cursorcol = col;
    cursorline = lin;
    pcursor(scol, slin);            /* direct positioning */
}

/*
 * Move a cursor within the current window.
 * Argument is:
 *  CCMOVELEFT, CCMOVERIGHT - one column to left or to right
 *  CCMOVEUP, CCMOVEDOWN    - one line up or down
 *  CCPGUP, CCPGDOWN        - page down or up
 *  CCHOME, CCEND           - to line start or end
 *  CCRETURN                - to start of next line
 *  CCTAB                   - to next tab stop
 *  0                       - no movement, check only
 */
void movecursor(arg)
    int arg;
{
    register int lin, col;

    lin = cursorline;
    col = cursorcol;
    switch (arg) {
    case 0:
        break;
    case CCHOME:                /* home: cursor to line start */
        col = - curwksp->offset;
        break;
    case CCMOVELEFT:            /* backspace */
        if (col + curwksp->offset > 0) {
            --col;
            break;
        }
        if (lin + curwksp->topline <= 0)
            break;
        lin--;
        /* fall through... */
    case CCEND:                 /* home: cursor to line end */
        if (clineno != curwksp->topline + lin)
            getlin(curwksp->topline + lin);
        col = cline_len - 1 - curwksp->offset;
        if (col >= 0 && col <= curwin->text_maxcol)
            break;
        curwksp->offset = cline_len - 1 - curwin->text_maxcol + defroffset;
        if (curwksp->offset < 0)
            curwksp->offset = 0;
        drawlines(0, curwin->text_maxrow);
        clineno = -1;
        getlin(curwksp->topline + lin);
        col = cline_len - 1 - curwksp->offset;
        break;
    case CCRETURN:              /* return */
        col = 0;
        /* fall through... */
    case CCMOVEDOWN:            /* move down 1 line */
        if (lin < curwin->text_maxrow)
            ++lin;
        else {
            putline();
            lin += curwksp->topline + 1;
            wksp_forward(curwin->text_maxrow / 2);
            lin -= curwksp->topline;
        }
        break;
    case CCMOVEUP:              /* move up 1 line */
        if (lin > 0)
            --lin;
        else if (curwksp->topline > 0) {
            putline();
            lin += curwksp->topline - 1;
            wksp_forward(- curwin->text_maxrow / 2);
            lin -= curwksp->topline;
        }
        break;
    case CCMOVERIGHT:           /* move forward */
        ++col;
        break;
    case CCTAB:                 /* tab */
        col += curwksp->offset;
        col = (col + 4) & ~3;
        col -= curwksp->offset;
        break;
    case CCPGDOWN:
        putline();
        wksp_forward(curwin->text_maxrow);
        lin = cursorline;
        break;
    case CCPGUP:
        putline();
        wksp_forward(- curwin->text_maxrow);
        lin = cursorline;
        break;
    }
    if (col > curwin->text_maxcol) {
        curwksp->offset += defroffset;
        col -= defroffset;
        drawlines(0, curwin->text_maxrow);
        clineno = -1;
    } else if (col < 0) {
        curwksp->offset -= defloffset - col;
        col = defloffset;
        if (curwksp->offset < 0) {
            col += curwksp->offset;
            curwksp->offset = 0;
        }
        drawlines(0, curwin->text_maxrow);
        clineno = -1;
    }

    if (lin < 0)
        lin = 0;
    else if (lin > curwin->text_maxrow)
        lin = curwin->text_maxrow;

    poscursor(col, lin);
}

/*
 * Put a symbol to current position.
 * When flag=1, count it to line size.
 */
void wputc(j, flg)
    int j, flg;
{
    if (flg && keysym != ' ') {
        if (curwin->firstcol[cursorline] > cursorcol)
            curwin->firstcol[cursorline] = cursorcol;
        if (curwin->lastcol[cursorline] <= cursorcol)
            curwin->lastcol[cursorline] = cursorcol + 1;
    }
    ++cursorcol;
    if (curwin->text_col + cursorcol >= NCOLS)
        cursorcol = - curwin->text_col;
    putch(j);
    if (cursorcol <= 0)
        poscursor(0,
            cursorline < 0 ? 0 :
            cursorline > curwin->text_maxrow ? 0 :
            cursorline);
    movecursor(0);
}

/*
 * Get a "Cmd:" parameter.
 * On return, param_type contains a type of entered parameter:
 *     0 -- no value entered.
 *    -1 -- text area defined. Coordinates of top left corner are saved in
 *          param_c0, param_r0, bottom right corner - in param_c1, param_r1.
 *     1 -- string value.  Allocated string is in param_str,
 *          length in param_len.  Old value of param_str is deallocated
 *          on a next call.
 */
char *param()
{
    register char *c1;
    char *cp, *c2;
    int c, ccol, cline, old_offset, old_topline;
    register int i, pn;
    window_t *w;
#define LPARAM 20       /* length increment */

    if (param_len != 0 && param_str != 0)
        free(param_str);
    param_c1 = param_c0 = cursorcol + curwksp->offset;
    param_r1 = param_r0 = cursorline + curwksp->topline;
    wputc(COCURS, 1);
    poscursor(cursorcol, cursorline);
    w = curwin;
    old_topline = curwksp->topline;
    old_offset = curwksp->offset;
back:
    telluser("Cmd: ", 0);
    win_switch(&paramwin);
    poscursor(5, 0);
    do {
        keysym = -1;
        getkeysym();
    } while (keysym == CCBACKSPACE);

    if (MOVECMD(keysym)) {
        telluser("*** Area defined by cursor ***", 0);
        win_switch(w);
        poscursor(param_c0 - curwksp->offset, param_r0 - curwksp->topline);
t0:
        while (MOVECMD(keysym)) {
            movecursor(keysym);
            if (cursorline + curwksp->topline == param_r0 &&
                cursorcol + curwksp->offset == param_c0)
                goto back;
            keysym = -1;
            getkeysym();
        }
        if (! CTRLCHAR(keysym) || keysym == CCBACKSPACE) {
            error("Printing character illegal here");
            keysym = -1;
            getkeysym();
            goto t0;
        }
        if (cursorcol + curwksp->offset > param_c0)
            param_c1 = cursorcol + curwksp->offset;
        else
            param_c0 = cursorcol + curwksp->offset;
        if (cursorline + curwksp->topline > param_r0)
            param_r1 = cursorline + curwksp->topline;
        else
            param_r0 = cursorline + curwksp->topline;
        param_len = 0;
        param_str = NULL;
        param_type = -1;

    } else if (CTRLCHAR(keysym)) {
        param_len = 0;
        param_str = NULL;
        param_type = 0;
    } else {
        param_len = pn = 0;
loop:
        c = getkeysym();
        if (pn >= param_len) {
            cp = param_str;
            param_str = salloc(param_len + LPARAM + 1); /* 1 for int_to_ext */
            c1 = param_str;
            c2 = cp;
            for (i=0; i<param_len; ++i)
                *c1++ = *c2++;
            if (param_len)
                free(cp);
            param_len += LPARAM;
        }

        if (keysym < ' ' || c==CCBACKSPACE || c==CCQUIT) {
            if (c == CCBACKSPACE && cursorcol != 0) {
                /* backspace */
                if (pn == 0) {
                    keysym = -1;
                    goto loop;
                }
                movecursor(CCMOVELEFT);
                --pn;
                if ((param_str[pn] & 0340) == 0) {
                    wputc(' ', 0);
                    movecursor(CCMOVELEFT);
                    movecursor(CCMOVELEFT);
                }
                param_str[pn] = 0;
                wputc(' ', 0);
                movecursor(CCMOVELEFT);
                keysym = -1;
                if (pn == 0)
                    goto back;
                goto loop;
            } else
                c = 0;
        }
        if (c == 0177)          /* del is a control code */
            c = 0;
        param_str[pn++] = c;
        if (c != 0) {
            if ((c & 0140) == 0){
                wputc('^', 0);
                c = c | 0100;
            }
            wputc(c, 0);
            keysym = -1;
            goto loop;
        }
        param_type = 1;
    }
    win_switch(w);
    cline = param_r0 - curwksp->topline;
    ccol = param_c0 - curwksp->offset;
    if (cline >= curwin->text_row && cline <= curwin->text_maxrow &&
        ccol >= curwin->text_col && ccol <= curwin->text_maxcol) {
        drawlines(cline, cline);
    } else {
        curwksp->topline = old_topline;
        curwksp->offset = old_offset;
        cline = param_r0 - curwksp->topline;
        ccol = param_c0 - curwksp->offset;
        drawlines(0, curwin->text_maxrow);
    }
    poscursor(ccol, cline);
    return (param_str);
}

/*
 * Draw borders for a window.
 * When vertf, draw a vertical borders.
 */
void win_borders(win, vertf)
    register window_t *win;
    int vertf;
{
#if MULTIWIN
    register int i;

    win_switch(&wholescreen);
    if (win->base_row != win->text_row) {
        poscursor(win->base_col, win->base_row);
        for (i = win->base_col; i <= win->max_col; i++)
            wputc(TMCH, 0);
    }
    if (vertf) {
        int j;
        for (j = win->base_row + 1; j <= win->max_row - 1; j++) {
            int c = win->leftbar[j - win->base_row - 1];
            if (c != 0) {
                poscursor(win->base_col, j);
                wputc(c, 0);
                poscursor(win->max_col, j);
                wputc(win->rightbar[j - win->base_row - 1], 0);
            }
        }
    }
    if (win->base_row != win->text_row) {
        poscursor(win->base_col, win->max_row);
        for (i = win->base_col; i <= win->max_col; i++)
            wputc(BMCH, 0);
    }
    /* poscursor(win->base_col + 1, win->base_row + 1); */
#endif
    win_switch(win);
}

/*
 * Display error message.
 */
void error(msg)
    char *msg;
{
    putch(COBELL);
    telluser("**** ", 0);
    telluser(msg, 5);
    message_displayed = 1;
}

/*
 * Display a message from column col.
 * When col=0 - clear the arg area.
 */
void telluser(msg, col)
    char *msg;
    int col;
{
    window_t *oldwin;
    register int c, l;

    oldwin = curwin;
    c = cursorcol;
    l = cursorline;
    win_switch(&paramwin);
    if (col == 0) {
        poscursor(0, 0);
        putblanks(paramwin.text_maxcol);
    }
    poscursor(col, 0);
    /* while (*msg) wputc(*msg++, 0); */
    wputs(msg, PARAMWIDTH);
    win_switch(oldwin);
    poscursor(c, l);
    dumpcbuf();
}

/*
 * Redraw a screen.
 */
void redisplay()
{
    register int i;
    int j;
    register window_t *curp, *curp0 = curwin;
    int col = cursorcol, lin = cursorline;

    /* Center the current line. */
    i = curwin->text_maxrow / 2;
    if (lin < i && curwksp->topline > i - lin) {
        curwksp->topline -= i - lin;
        lin = i;
    } else if (lin > i) {
        j = file[curfile].nlines - curwin->text_maxrow;
        if (curwksp->topline < j) {
            curwksp->topline += lin - i;
            lin = i;
            if (curwksp->topline > j) {
                lin += curwksp->topline - j;
                curwksp->topline = j;
            }
        }
    }
    win_switch(&wholescreen);
    cursorcol = cursorline = 0;
    putch(COFIN);
    putch(COSTART);
    putch(COHO);
    for (j=0; j<nwinlist; j++) {
        win_switch(winlist[j]);
        curp = curwin;
        for (i=0; i<curp->text_maxrow+1; i++) {
            curp->firstcol[i] = 0;
            curp->lastcol[i] = 0; /* curwin->text_maxcol;*/
            curp->leftbar[i] = ' ';
            curp->rightbar[i] = ' ';
        }
        win_borders(curp, 0);
        drawlines(0, curp->text_maxrow);
    }
    win_switch(curp0);
    poscursor(col, lin);
}

/*
 * Put a string, limited by column.
 */
void wputs(ss, ml)
    char *ss;
    int ml;
{
    register char *s = ss;

    while (*s && cursorcol < ml)
        wputc(*s++, 0);
}
