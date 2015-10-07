/*
 * Draw a circle (no fill).
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

void gpanel_circle(int color, int x, int y, int radius)
{
    struct gpanel_circle_t param;

    param.color = color;
    param.x = x;
    param.y = y;
    param.radius = radius;
    ioctl(_gpanel_fd, GPANEL_CIRCLE, &param);
}
