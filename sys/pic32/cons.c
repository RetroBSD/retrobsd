/*
 * UART driver for PIC32.
 *
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "conf.h"
#include "user.h"
#include "ioctl.h"
#include "tty.h"
#include "systm.h"

#include "uart.h"
#include "usb_uart.h"

#define CONCAT(x,y) x ## y
#define BBAUD(x) CONCAT(B,x)

#ifndef CONSOLE_BAUD
#define CONSOLE_BAUD 115200
#endif

const struct devspec cndevs[] = {
    { 0, "console" },
    { 0, 0 }
};

dev_t console_device = -1;

#define NKL     1                       /* Only one console device */

#define Q2(X) #X
#define QUOTE(X) Q2(X)

struct tty cnttys [1];

void cnstart (struct tty *tp);

void cninit()
{
    console_device = get_cdev_by_name(QUOTE(CONSOLE_DEVICE));
}

void cnidentify()
{
        printf ("console: %s (%d,%d)\n",
            cdevname(console_device), major(console_device), minor(console_device));
}

int cnopen(dev_t dev, int flag, int mode)
{
    return cdevsw[major(console_device)].d_open(console_device, flag, mode);
}

int cnclose (dev_t dev, int flag, int mode)
{
    return cdevsw[major(console_device)].d_close(console_device, flag, mode);
}

int cnread(dev_t dev,register struct uio *uio, int flag)
{
    return cdevsw[major(console_device)].d_read(console_device, uio, flag);
}

int cnwrite(dev_t dev,register struct uio *uio, int flag)
{
    return cdevsw[major(console_device)].d_write(console_device, uio, flag);
}

int cnselect(dev_t dev, int rw)
{
    return cdevsw[major(console_device)].d_select(console_device, rw);
}

int cnioctl(dev_t dev, register u_int cmd, caddr_t addr, int flag)
{
    return cdevsw[major(console_device)].d_ioctl(console_device, cmd, addr, flag);
}

/*
 * Put a symbol on console terminal.
 */
void cnputc(char c)
{
    if (cdevsw[major(console_device)].r_write) {
        cdevsw[major(console_device)].r_write(console_device, c);
    } else {
        putc(c, &cdevsw[major(console_device)].d_ttys[minor(console_device)].t_outq);
        ttstart(&cdevsw[major(console_device)].d_ttys[minor(console_device)]);
        ttyflush(&cdevsw[major(console_device)].d_ttys[minor(console_device)],0);
    }
    if(c=='\n')
        cnputc('\r');
}

/*
 * Receive a symbol from console terminal.
 */
int
cngetc ()
{
    if (cdevsw[major(console_device)].r_read) {
        return cdevsw[major(console_device)].r_read(console_device);
    } else {
        return getc(&cdevsw[major(console_device)].d_ttys[minor(console_device)].t_rawq);
    }
}
