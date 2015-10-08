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

struct tty cnttys [1];

int cnopen(dev_t cn, int flag, int mode)
{
    dev_t dev = makedev(CONS_MAJOR, CONS_MINOR);

    return cdevsw[CONS_MAJOR].d_open(dev, flag, mode);
}

int cnclose (dev_t cn, int flag, int mode)
{
    dev_t dev = makedev(CONS_MAJOR, CONS_MINOR);

    return cdevsw[CONS_MAJOR].d_close(dev, flag, mode);
}

int cnread(dev_t cn, struct uio *uio, int flag)
{
    dev_t dev = makedev(CONS_MAJOR, CONS_MINOR);

    return cdevsw[CONS_MAJOR].d_read(dev, uio, flag);
}

int cnwrite(dev_t cn, struct uio *uio, int flag)
{
    dev_t dev = makedev(CONS_MAJOR, CONS_MINOR);

    return cdevsw[CONS_MAJOR].d_write(dev, uio, flag);
}

int cnselect(dev_t cn, int rw)
{
    dev_t dev = makedev(CONS_MAJOR, CONS_MINOR);

    return cdevsw[CONS_MAJOR].d_select(dev, rw);
}

int cnioctl(dev_t cn, u_int cmd, caddr_t addr, int flag)
{
    dev_t dev = makedev(CONS_MAJOR, CONS_MINOR);

    return cdevsw[CONS_MAJOR].d_ioctl(dev, cmd, addr, flag);
}

/*
 * Put a symbol on console terminal.
 */
void cnputc(char c)
{
    if (cdevsw[CONS_MAJOR].r_write) {
        dev_t dev = makedev(CONS_MAJOR, CONS_MINOR);

        cdevsw[CONS_MAJOR].r_write(dev, c);
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
int cngetc()
{
    if (cdevsw[CONS_MAJOR].r_read) {
        dev_t dev = makedev(CONS_MAJOR, CONS_MINOR);

        return cdevsw[CONS_MAJOR].r_read(dev);
    } else {
        return getc(&cdevsw[CONS_MAJOR].d_ttys[CONS_MINOR].t_rawq);
    }
}
