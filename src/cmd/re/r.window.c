/*
 * Implementing windows.
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include "r.defs.h"

/*
 * Move a current window by nl rows down.
 */
void wksp_forward(nl)
    int nl;
{
    register int cc, cl;

    if (nl < 0) {
        if (curwksp->topline == 0) {
            if (cursorline != 0)
                poscursor(cursorcol, 0);
            return;
        }
    } else {
        int last_line = file[curfile].nlines - curwksp->topline;
        if (last_line <= curwin->text_maxrow) {
            if (cursorline != last_line)
                poscursor(cursorcol, last_line);
            return;
        }
    }
    curwksp->topline += nl;
    if (curwksp->topline > file[curfile].nlines - curwin->text_maxrow)
        curwksp->topline = file[curfile].nlines - curwin->text_maxrow;
    if (curwksp->topline < 0)
        curwksp->topline = 0;
    cc = cursorcol;
    cl = cursorline;
    drawlines(0, curwin->text_maxrow);
    poscursor(cc, cl);
}

/*
 * Shift a text view by nc columns to right.
 */
void wksp_offset(nc)
    int nc;
{
    register int cl, cc;

    cl = cursorline;
    cc = cursorcol;
    if ((curwksp->offset + nc) < 0)
        nc = - curwksp->offset;
    curwksp->offset += nc;
    drawlines(0, curwin->text_maxrow);
    cc -= nc;
    if (cc < 0)
        cc = 0;
    else if (cc > curwin->text_maxcol)
        cc = curwin->text_maxcol;
    poscursor(cc, cl);
}

/*
 * Go to line by number.
 */
void gtfcn(number)
    int number;
{
    register int i;

    wksp_forward(number - curwksp->topline - curwin->text_maxrow / 2);
    i = number - curwksp->topline;
    if (i >= 0) {
        if (i > curwin->text_maxrow)
            i = curwin->text_maxrow;
        poscursor(cursorcol, i);
    }
}

/*
 * Scroll window to show a given area.
 * slin - line number with a cursor tag, to erase
 * ok_to_move=0 - do not move the window, when possible
 */
void cgoto(ln, col, slin, ok_to_move)
    int ln, col, slin, ok_to_move;
{
    register int lin;

    lin = ln - curwksp->topline;
    if (ok_to_move || lin < 0 || lin  > curwin->text_maxrow) {
        ok_to_move = -1;
        lin = curwin->text_maxrow / 2;
        curwksp->topline = ln - lin;
        if (curwksp->topline < 0) {
            lin += curwksp->topline;
            curwksp->topline = 0;
        }
    }
    col -= curwksp->offset;
    if (col < 0 || col > curwin->text_maxcol) {
        curwksp->offset += col;
        col = 0;
        ok_to_move = -1;
    }
    if (ok_to_move)
        drawlines(0, curwin->text_maxrow);
    else if (slin >=0)
        drawlines(slin, slin);

    poscursor(col, lin);
}


/*
 * Switch to given window.
 * Compute cursorcol, cursorline for new window.
 */
void win_switch(ww)
    window_t *ww;
{
    register window_t *w = ww;

    cursorcol  -= (w->text_col - curwin->text_col);
    cursorline -= (w->text_row - curwin->text_row);
    curwin = w;
    curwksp = curwin->wksp;
    if (curwksp)
        curfile = curwksp->wfile;
}

/*
 * Create new window.
 * Flag need_borders = 1 when borders enable.
 */
void win_create(w, col_left, col_right, row_top, row_bottom, need_borders)
    register window_t *w;
    int col_left, col_right, row_top, row_bottom, need_borders;
{
    register int i, size;

    w->base_col = col_left;
    w->max_col  = col_right;
    w->base_row = row_top;
    w->max_row  = row_bottom;
    if (need_borders) {
        w->text_col    = col_left + 1;
        w->text_row    = row_top + 1;
        w->text_maxcol = col_right - col_left - 2;
        w->text_maxrow = row_bottom - row_top - 2;
    } else {
        w->text_col    = col_left;
        w->text_row    = row_top;
        w->text_maxcol = col_right - col_left;
        w->text_maxrow = row_bottom - row_top;
    }

    w->wksp = (workspace_t*) salloc(sizeof(workspace_t));
    w->altwksp = (workspace_t*) salloc(sizeof(workspace_t));

    /* eventually this extra space may not be needed */
    size = NLINES - row_top + 1;
    w->firstcol = (unsigned char*) salloc(size);
    for (i=0; i<size; i++)
        w->firstcol[i] = w->text_maxcol + 1;
    w->lastcol = (unsigned char*) salloc(size);
    w->leftbar = salloc(size);
    w->rightbar = salloc(size);
    w->wksp->cursegm = file[2].chain; /* "#", as it's always available */
}

/*
 * Make new window.
 */
void win_open(file)
    char *file;
{
    register window_t *oldwin, *win;
    int winnum;

    /* Any space for a new window? */
    if (nwinlist >= MAXWINLIST) {
        error("Can't make any more windows.");
        return;
    }
    win = winlist[nwinlist++] = (window_t*) salloc(sizeof(window_t));

    /* Find a window number. */
    oldwin = curwin;
    for (winnum=0; winlist[winnum] != oldwin; winnum++);
    win->prevwinnum = winnum;

#if MULTIWIN
    /*
     * Split the window into two, horizontally or vertically.
     */
    if (cursorcol == 0 && cursorline > 0
        && cursorline < oldwin->text_maxrow)
    {
        /* Split horizontally. */
        register int i;
        win_create(win, oldwin->base_col, oldwin->max_col,
            oldwin->base_row + cursorline + 1, oldwin->max_row, 1);
        oldwin->max_row = oldwin->base_row + cursorline + 1;
        oldwin->text_maxrow = cursorline - 1;
        for (i=0; i <= win->text_maxrow; i++) {
            win->firstcol[i] = oldwin->firstcol[i + cursorline + 1];
            win->lastcol[i] = oldwin->lastcol[i + cursorline + 1];
        }
    } else if (cursorline == 0 && cursorcol > 0
        && cursorcol < oldwin->text_maxcol-1)
    {
        /* Split vertically. */
        register int i;
        win_create(win, oldwin->base_col + cursorcol + 2, oldwin->max_col,
            oldwin->base_row, oldwin->max_row, 1);
        oldwin->max_col = oldwin->base_col + cursorcol + 1;
        oldwin->text_maxcol = cursorcol - 1;
        for (i=0; i <= win->text_maxrow; i++) {
            if (oldwin->lastcol[i] > oldwin->text_maxcol + 1) {
                win->firstcol[i] = 0;
                win->lastcol[i] = oldwin->lastcol[i] - cursorcol - 2;
                oldwin->lastcol[i] = oldwin->text_maxcol + 1;
                oldwin->rightbar[i] = MRMCH;
            }
        }
    } else {
        error("Can't put a window there.");
        nwinlist--;
        free(win);
        return;
    }
#else
    /*
     * Every window occupies the entire screen.
     */
    win_create(win, oldwin->base_col, oldwin->max_col,
        oldwin->base_row, oldwin->max_row, 1);
#endif
    oldwin->wksp->cursorcol = oldwin->altwksp->cursorcol = 0;
    oldwin->wksp->cursorrow = oldwin->altwksp->cursorrow = 0;
    win_switch(win);
    //defplline = defmiline = (win->max_row - win->base_row)/ 4 + 1;
    if (editfile (file, 0, 0, 1, 1) <= 0 &&
        editfile (deffile, 0, 0, 0, 1) <= 0)
        error("Cannot open help file.");
    win_borders(oldwin, 1);
    win_borders(win, 1);
    poscursor(0, 0);
}

/*
 * Remove last window.
 */
void win_remove()
{
    int j, pwinnum;
    register int i;
    register window_t *win, *pwin;

    if (nwinlist == 1) {
        error ("Can't remove the last window.");
        return;
    }
    win = winlist[--nwinlist];
    pwinnum = win->prevwinnum;
    pwin = winlist[pwinnum];
    if (pwin->max_row != win->max_row) {
        /* Vertical */
        pwin->firstcol[j = pwin->text_maxrow+1] = 0;
        pwin->lastcol[j++] = pwin->text_maxcol+1;
        for (i=0; i<=win->text_maxrow; i++) {
            pwin->firstcol[j+i] = win->firstcol[i];
            pwin->lastcol[j+i] = win->lastcol[i];
        }
        pwin->max_row = win->max_row;
        pwin->text_maxrow = pwin->max_row - pwin->base_row - 2;
    } else {
        /* Horizontal */
        for (i=0; i<=pwin->text_maxrow; i++) {
            pwin->lastcol[i] = win->lastcol[i] +
                win->base_col - pwin->base_col;
            if (pwin->firstcol[i] > pwin->text_maxcol)
                pwin->firstcol[i] = pwin->text_maxcol;
        }
        pwin->max_col = win->max_col;
        pwin->text_maxcol = pwin->max_col - pwin->base_col - 2;
    }
    win_borders(pwin, 1);
    win_goto(pwinnum);
    drawlines(0, curwin->text_maxrow);
    poscursor(0, 0);
    DEBUGCHECK;
    free(win->firstcol);
    free(win->lastcol);
    free(win->leftbar);
    free(win->rightbar);
    free((char *)win->wksp);
    free((char *)win->altwksp);
    free((char *)win);
    DEBUGCHECK;
}

/*
 * Switch to another window by number.
 * When number is -1, select a next window after the curwin.
 */
void win_goto(winnum)
    int winnum;
{
    register window_t *oldwin, *win;

    oldwin = curwin;
    oldwin->wksp->cursorcol = cursorcol;
    oldwin->wksp->cursorrow = cursorline;
    if (winnum < 0) {
        /* Find a window next to the current one. */
        for (winnum=0; winnum<nwinlist; winnum++) {
            if (oldwin == winlist[winnum]) {
                winnum++;
                break;
            }
        }
        if (winnum >= nwinlist)
            winnum = 0;
    }
    win = winlist[winnum];
    if (win == oldwin)
        return;

    /* win_borders(oldwin, win); */
    win_switch(win);
    //defplline = defmiline = (win->max_row - win->base_row) / 4 + 1;
    poscursor(curwin->wksp->cursorcol, curwin->wksp->cursorrow);
}

/*
 * Redisplay changed windows after lines in file shifted.
 *
 * w - workspace;
 * fn - index of changed file;
 * from, to, delta - ranged of changed lines and a number of lines in it
 *
 * Actions:
 * 1. Redraw all windows, overlapped with the given area;
 * 2. Changed workspaces get updated segm pointers;
 * 3. Line numbers updated in workspaces, containing the tails
 *    of changed files.
 */
void wksp_redraw(w, fn, from, to, delta)
    workspace_t *w;
    int from, to, delta, fn;
{
    register workspace_t *tw;
    register int i, j;
    int k, l, m;
    window_t *owin;

    for (i = 0; i < nwinlist; i++) {
        tw = winlist[i]->altwksp;
        if (tw->wfile == fn && tw != w) {
            /* Update segm pointer. */
            tw->line = tw->segmline = 0;
            tw->cursegm = file[fn].chain;

            /* Update a line number */
            j = delta >= 0 ? to : from;
            if (tw->topline > j)
                tw->topline += delta;
        }
        tw = winlist[i]->wksp;
        if (tw->wfile == fn && tw != w) {
            /* Update a segm pointer. */
            tw->line = tw->segmline = 0;
            tw->cursegm = file[fn].chain;

            /* Update a line number */
            j = (delta >= 0) ? to : from;
            if (tw->topline > j)
                tw->topline += delta;

            /* Redraw the window, if changed */
            j = (from > tw->topline) ? from : tw->topline;
            k = tw->topline + winlist[i]->text_maxrow;
            if (k > to)
                k = to;
            if (j <= k) {
                owin = curwin;
                l = cursorcol;
                m = cursorline;
                win_switch(winlist[i]);
                drawlines(j - tw->topline,
                    delta == 0 ? k - tw->topline : winlist[i]->text_maxrow);
                win_switch(owin);
                poscursor(l, m);
            }
        }
    }
}
