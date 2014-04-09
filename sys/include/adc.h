/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)adc.h	1.4 (2.11BSD GTE) 1997/3/28
 */

#ifndef	_ADC_H
#define _ADC_H

#include <sys/ioctl.h>
#include <sys/uio.h>

#define ADCMAX 15

#ifdef KERNEL
#include "conf.h"

extern const struct devspec adcdevs[];

int adc_open (dev_t dev, int flag, int mode);
int adc_close (dev_t dev, int flag, int mode);
int adc_read (dev_t dev, struct uio *uio, int flag);
int adc_write (dev_t dev, struct uio *uio, int flag);
int adc_ioctl (dev_t dev, u_int cmd, caddr_t addr, int flag);
#endif

#endif
