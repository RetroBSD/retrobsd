/*
 * Device open and close routines.
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
#include <unistd.h>
#include <fcntl.h>
#include <sys/gpanel.h>

int _gpanel_fd = -1;

/*
 * Open a graphics panel device.
 */
int gpanel_open(const char *devname)
{
    _gpanel_fd = open(devname, O_RDWR);
    if (_gpanel_fd < 0)
        return -1;
    return 0;
}

/*
 * Close gpanel device.
 */
void gpanel_close(void)
{
    if (_gpanel_fd >= 0) {
        close(_gpanel_fd);
        _gpanel_fd = -1;
    }
}
