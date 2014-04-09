/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)gpio.h	1.4 (2.11BSD GTE) 1997/3/28
 */

/*
 * Ioctl definitions
 */
#ifndef	_GPIO_H
#define _GPIO_H

#include <sys/ioctl.h>
#include <sys/uio.h>

/* control general-purpose i/o pins */
#define GPIO_PORT(n)	((n) & 0xff)                    /* port number */
#define GPIO_PORTA	GPIO_PORT(0)
#define GPIO_PORTB	GPIO_PORT(1)
#define GPIO_PORTC	GPIO_PORT(2)
#define GPIO_PORTD	GPIO_PORT(3)
#define GPIO_PORTE	GPIO_PORT(4)
#define GPIO_PORTF	GPIO_PORT(5)
#define GPIO_PORTG	GPIO_PORT(6)
#define GPIO_COMMAND	0x1fff0000                      /* command mask */
#define GPIO_CONFIN	(IOC_VOID | 1 << 16 | 'g'<<8)   /* configure as input */
#define GPIO_CONFOUT    (IOC_VOID | 1 << 17 | 'g'<<8)   /* configure as output */
#define GPIO_CONFOD	(IOC_VOID | 1 << 18 | 'g'<<8)   /* configure as open drain */
#define GPIO_DECONF	(IOC_VOID | 1 << 19 | 'g'<<8)   /* deconfigure */
#define GPIO_STORE	(IOC_VOID | 1 << 20 | 'g'<<8)   /* store all outputs */
#define GPIO_SET	(IOC_VOID | 1 << 21 | 'g'<<8)   /* set to 1 by mask */
#define GPIO_CLEAR	(IOC_VOID | 1 << 22 | 'g'<<8)   /* set to 0 by mask */
#define GPIO_INVERT	(IOC_VOID | 1 << 23 | 'g'<<8)   /* invert by mask */
#define GPIO_POLL	(IOC_VOID | 1 << 24 | 'g'<<8)   /* poll */
#define GPIO_LOL	(IOC_IN   | 1 << 25 | 'g'<<8)   /* display lol picture */

#ifdef KERNEL

#include "conf.h"

extern const struct devspec gpiodevs[];

int gpioopen (dev_t dev, int flag, int mode);
int gpioclose (dev_t dev, int flag, int mode);
int gpioread (dev_t dev, struct uio *uio, int flag);
int gpiowrite (dev_t dev, struct uio *uio, int flag);
int gpioioctl (dev_t dev, u_int cmd, caddr_t addr, int flag);
#endif

#endif
