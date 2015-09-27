/*
 * UART driver for PIC32.
 *
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <machine/uart.h>
#include <machine/usb_uart.h>

dev_t console_device = -1;

struct tty cnttys [1];

void cninit()
{
    console_device = makedev(CONS_MAJOR, CONS_MINOR);
}

void cnidentify()
{
    //printf ("console: %s (%d,%d)\n", cdevname(console_device),
    //    CONS_MAJOR, CONS_MINOR);
}

int cnopen(dev_t dev, int flag, int mode)
{
    return cdevsw[CONS_MAJOR].d_open(console_device, flag, mode);
}

int cnclose (dev_t dev, int flag, int mode)
{
    return cdevsw[CONS_MAJOR].d_close(console_device, flag, mode);
}

int cnread(dev_t dev,register struct uio *uio, int flag)
{
    return cdevsw[CONS_MAJOR].d_read(console_device, uio, flag);
}

int cnwrite(dev_t dev,register struct uio *uio, int flag)
{
    return cdevsw[CONS_MAJOR].d_write(console_device, uio, flag);
}

int cnselect(dev_t dev, int rw)
{
    return cdevsw[CONS_MAJOR].d_select(console_device, rw);
}

int cnioctl(dev_t dev, register u_int cmd, caddr_t addr, int flag)
{
    return cdevsw[CONS_MAJOR].d_ioctl(console_device, cmd, addr, flag);
}

/*
 * Put a symbol on console terminal.
 */
void cnputc(char c)
{
    if (cdevsw[CONS_MAJOR].r_write) {
        cdevsw[CONS_MAJOR].r_write(console_device, c);
    } else {
        putc(c, &cdevsw[CONS_MAJOR].d_ttys[CONS_MINOR].t_outq);
        ttstart(&cdevsw[CONS_MAJOR].d_ttys[CONS_MINOR]);
        ttyflush(&cdevsw[CONS_MAJOR].d_ttys[CONS_MINOR],0);
    }
    if(c=='\n')
        cnputc('\r');
}

/*
 * Receive a symbol from console terminal.
 */
int cngetc ()
{
    if (cdevsw[CONS_MAJOR].r_read) {
        return cdevsw[CONS_MAJOR].r_read(console_device);
    } else {
        return getc(&cdevsw[CONS_MAJOR].d_ttys[CONS_MINOR].t_rawq);
    }
}
