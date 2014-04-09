/*
 * Console driver via USB.
 *
 * Copyright (C) 2011 Serge Vakulenko, <serge@vak.ru>
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
#ifndef _USB_UART_H
#define _USB_UART_H

#include "conf.h"

extern const struct devspec usbdevs[];
extern unsigned int usb_major;

#define USB_MAJOR 13

extern struct tty usbttys[1];
extern void usbinit();
extern int usbopen(dev_t dev, int flag, int mode);
extern int usbclose(dev_t dev, int flag, int mode);
extern int usbread(dev_t dev, struct uio *uio, int flag);
extern int usbwrite(dev_t dev, struct uio *uio, int flag);
extern int usbioctl(dev_t dev, register u_int cmd, caddr_t addr, int flag);
extern int usbselect(dev_t dev, int rw);
extern void usbstart (register struct tty *tp);
extern void usbputc(dev_t dev, char c);
extern char usbgetc(dev_t dev);
extern void usbintr(int chan);

#endif
