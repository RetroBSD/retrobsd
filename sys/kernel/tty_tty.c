/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *  @(#)tty_tty.c   1.2 (2.11BSD GTE) 11/29/94
 */

/*
 * Indirect driver for controlling tty.
 *
 */
#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>

/*ARGSUSED*/
int
syopen (dev_t dev, int flag)
{
    if (u.u_ttyp == NULL)
        return (ENXIO);
    return((*cdevsw[major(u.u_ttyd)].d_open)(u.u_ttyd, flag, 0));
}

/*ARGSUSED*/
int
syread (dev_t dev, struct uio *uio, int flag)
{
    if (u.u_ttyp == NULL)
        return (ENXIO);
    return ((*cdevsw[major(u.u_ttyd)].d_read)(u.u_ttyd, uio, flag));
}

/*ARGSUSED*/
int
sywrite (dev_t dev, struct uio *uio, int flag)
{
    if (u.u_ttyp == NULL)
        return (ENXIO);
    return ((*cdevsw[major(u.u_ttyd)].d_write)(u.u_ttyd, uio, flag));
}

/*ARGSUSED*/
int
syioctl (dev_t dev, u_int cmd, caddr_t addr, int flag)
{
    if (cmd == TIOCNOTTY) {
        u.u_ttyp = 0;
        u.u_ttyd = 0;
        u.u_procp->p_pgrp = 0;
        return (0);
    }
    if (u.u_ttyp == NULL)
        return (ENXIO);
    return ((*cdevsw[major(u.u_ttyd)].d_ioctl)(u.u_ttyd, cmd, addr, flag));
}

/*ARGSUSED*/
int
syselect (dev_t dev, int flag)
{

    if (u.u_ttyp == NULL) {
        u.u_error = ENXIO;
        return (0);
    }
    return ((*cdevsw[major(u.u_ttyd)].d_select)(u.u_ttyd, flag));
}
