/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tty_tty.c	1.2 (2.11BSD GTE) 11/29/94
 */

/*
 * Indirect driver for controlling tty.
 *
 */
#include "param.h"
#include "user.h"
#include "proc.h"
#include "ioctl.h"
#include "tty.h"
#include "conf.h"

const struct devspec sydevs[] = {
    { 0, "tty" },
    { 0, 0 }
};

/*ARGSUSED*/
int
syopen (dev, flag)
	dev_t dev;
	int flag;
{
	if (u.u_ttyp == NULL)
		return (ENXIO);
	return((*cdevsw[major(u.u_ttyd)].d_open)(u.u_ttyd, flag, 0));
}

/*ARGSUSED*/
int
syread (dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	if (u.u_ttyp == NULL)
		return (ENXIO);
	return ((*cdevsw[major(u.u_ttyd)].d_read)(u.u_ttyd, uio, flag));
}

/*ARGSUSED*/
int
sywrite (dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	if (u.u_ttyp == NULL)
		return (ENXIO);
	return ((*cdevsw[major(u.u_ttyd)].d_write)(u.u_ttyd, uio, flag));
}

/*ARGSUSED*/
int
syioctl (dev, cmd, addr, flag)
	dev_t dev;
	u_int cmd;
	caddr_t addr;
	int flag;
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
syselect (dev, flag)
	dev_t dev;
	int flag;
{

	if (u.u_ttyp == NULL) {
		u.u_error = ENXIO;
		return (0);
	}
	return ((*cdevsw[major(u.u_ttyd)].d_select)(u.u_ttyd, flag));
}
