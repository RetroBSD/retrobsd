/*
 * Fill a triangle.
 * Code ported from AdaFruit TFT LCD library.
 *
 * Copyright (C) 2015 Serge Vakulenko, <serge@vak.ru>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#include <sys/ioctl.h>
#include <sys/gpanel.h>

/*
 * Swap values of two integer variables.
 */
#define swapi(x,y) { int _t = x; x = y; y = _t; }

void gpanel_fill_triangle(int color, int x0, int y0,
    int x1, int y1, int x2, int y2)
{
    int a, b, y, last;

    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1) {
        swapi(y0, y1);
        swapi(x0, x1);
    }
    if (y1 > y2) {
        swapi(y2, y1);
        swapi(x2, x1);
    }
    if (y0 > y1) {
        swapi(y0, y1);
        swapi(x0, x1);
    }

    if (y0 == y2) {
        // Handle awkward all-on-same-line case as its own thing
        a = b = x0;

        if (x1 < a)
            a = x1;
        else if (x1 > b)
            b = x1;

        if (x2 < a)
            a = x2;
        else if (x2 > b)
            b = x2;

        gpanel_fill(color, a, y0, b, y0);
        return;
    }

    int dx01 = x1 - x0;
    int dy01 = y1 - y0;
    int dx02 = x2 - x0;
    int dy02 = y2 - y0;
    int dx12 = x2 - x1;
    int dy12 = y2 - y1;
    int sa = 0;
    int sb = 0;

    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y1 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y0=y1
    // (flat-topped triangle).
    if (y1 == y2)
        last = y1;      // Include y1 scanline
    else
        last = y1-1;    // Skip it

    for (y=y0; y<=last; y++) {
        a   = x0 + sa / dy01;
        b   = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;

        /* longhand:
         * a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
         * b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
         */
        if (a > b)
            swapi(a, b);

        gpanel_fill(color, a, y, b, y);
    }

    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y1=y2.
    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for (; y<=y2; y++) {
        a   = x1 + sa / dy12;
        b   = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;

        /* longhand:
         * a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
         * b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
         */
        if (a > b)
            swapi(a, b);

        gpanel_fill(color, a, y, b, y);
    }
}
