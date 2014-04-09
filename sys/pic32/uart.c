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

#define CONCAT(x,y) x ## y
#define BBAUD(x) CONCAT(B,x)

unsigned int uart_major = 0;

#ifndef UART1_BAUD
#define UART1_BAUD 115200
#endif
#ifndef UART2_BAUD
#define UART2_BAUD 115200
#endif
#ifndef UART3_BAUD
#define UART3_BAUD 115200
#endif
#ifndef UART4_BAUD
#define UART4_BAUD 115200
#endif
#ifndef UART5_BAUD
#define UART5_BAUD 115200
#endif
#ifndef UART6_BAUD
#define UART6_BAUD 115200
#endif

const struct uart_irq uirq[NUART] = {
	{ PIC32_IRQ_U1E, PIC32_IRQ_U1RX, PIC32_IRQ_U1TX },
	{ PIC32_IRQ_U2E, PIC32_IRQ_U2RX, PIC32_IRQ_U2TX },
	{ PIC32_IRQ_U3E, PIC32_IRQ_U3RX, PIC32_IRQ_U3TX },
	{ PIC32_IRQ_U4E, PIC32_IRQ_U4RX, PIC32_IRQ_U4TX },
	{ PIC32_IRQ_U5E, PIC32_IRQ_U5RX, PIC32_IRQ_U5TX },
	{ PIC32_IRQ_U6E, PIC32_IRQ_U6RX, PIC32_IRQ_U6TX },
};

struct tty uartttys[NUART];

static unsigned speed_bps [NSPEEDS] = {
    0,      50,     75,     150,    200,    300,    600,    1200,
    1800,   2400,   4800,   9600,   19200,  38400,  57600,  115200,
    230400, 460800, 500000, 576000, 921600, 1000000, 1152000, 1500000,
    2000000, 2500000, 3000000, 3500000, 4000000
};

const struct devspec uartdevs[] = {
    { 0, "tty0" },
    { 1, "tty1" },
    { 2, "tty2" },
    { 3, "tty3" },
    { 4, "tty4" },
    { 5, "tty5" },
    { 0, 0 },
};

void cnstart (struct tty *tp);
struct uartreg *uart[NUART] = {
	(struct uartreg*) &U1MODE,
	(struct uartreg*) &U2MODE,
	(struct uartreg*) &U3MODE,
	(struct uartreg*) &U4MODE,
	(struct uartreg*) &U5MODE,
	(struct uartreg*) &U6MODE
};

void uartinit()
{
	unsigned char unit;
    int i;

    // Our first task is to find out what our major
    // number is, so that the console can continue to work.

    for (i=0; i<nchrdev; i++) {
        if (cdevsw[i].d_open == uartopen) {
            uart_major = i;
        }
    }

	/*
	 * Setup UART registers.
	 * Compute the divisor for 115.2 kbaud.
	 */
	for(unit=0; unit<NUART; unit++)
	{

		// SPI2 is U3/U6 (2/5)
		// SPI3 is U1/U4 (0/3)
		// SPI4 is U2/U5 (1/4)

#ifndef UART1_ENABLED
        if (unit==0)
            continue;
#endif
#ifndef UART2_ENABLED
        if (unit==1)
            continue;
#endif
#ifndef UART3_ENABLED
        if (unit==2)
            continue;
#endif
#ifndef UART4_ENABLED
        if (unit==3)
            continue;
#endif
#ifndef UART5_ENABLED
        if (unit==4)
            continue;
#endif
#ifndef UART6_ENABLED
        if (unit==5)
            continue;
#endif

		if ((SD0_PORT == 2) && (unit==2 || unit==5))
			continue;
		if ((SD0_PORT == 3) && (unit==0 || unit==3))
			continue;
		if ((SD0_PORT == 4) && (unit==1 || unit==4))
			continue;

        switch(unit)
        {
            case 0:
#ifdef UART1_ENA_PORT
	        /* Enable UART1 phy - pin is assumed to be active low */
	        TRIS_CLR(UART1_ENA_PORT) = 1 << UART1_ENA_PIN;
		LAT_CLR(UART1_ENA_PORT) = 1 << UART1_ENA_PIN;
		udelay(2500);
#endif
                uart[unit]->brg = PIC32_BRG_BAUD (BUS_KHZ * 1000, UART1_BAUD);
                break;
            case 1:
#ifdef UART2_ENA_PORT
	        /* Enable UART2 phy - pin is assumed to be active low */
	        TRIS_CLR(UART2_ENA_PORT) = 1 << UART2_ENA_PIN;
		LAT_CLR(UART2_ENA_PORT) = 1 << UART2_ENA_PIN;
		udelay(2500);
#endif
                uart[unit]->brg = PIC32_BRG_BAUD (BUS_KHZ * 1000, UART2_BAUD);
                break;
            case 2:
#ifdef UART3_ENA_PORT
	        /* Enable UART3 phy - pin is assumed to be active low */
	        TRIS_CLR(UART3_ENA_PORT) = 1 << UART3_ENA_PIN;
		LAT_CLR(UART3_ENA_PORT) = 1 << UART3_ENA_PIN;
		udelay(2500);
#endif
                uart[unit]->brg = PIC32_BRG_BAUD (BUS_KHZ * 1000, UART3_BAUD);
                break;
            case 3:
#ifdef UART4_ENA_PORT
	        /* Enable UART4 phy - pin is assumed to be active low */
	        TRIS_CLR(UART4_ENA_PORT) = 1 << UART4_ENA_PIN;
		LAT_CLR(UART4_ENA_PORT) = 1 << UART4_ENA_PIN;
		udelay(2500);
#endif
                uart[unit]->brg = PIC32_BRG_BAUD (BUS_KHZ * 1000, UART4_BAUD);
                break;
            case 4:
#ifdef UART5_ENA_PORT
	        /* Enable UART5 phy - pin is assumed to be active low */
	        TRIS_CLR(UART5_ENA_PORT) = 1 << UART5_ENA_PIN;
		LAT_CLR(UART5_ENA_PORT) = 1 << UART5_ENA_PIN;
		udelay(2500);
#endif
                uart[unit]->brg = PIC32_BRG_BAUD (BUS_KHZ * 1000, UART5_BAUD);
                break;
            case 5:
#ifdef UART6_ENA_PORT
	        /* Enable UART6 phy - pin is assumed to be active low */
	        TRIS_CLR(UART6_ENA_PORT) = 1 << UART6_ENA_PIN;
		LAT_CLR(UART6_ENA_PORT) = 1 << UART6_ENA_PIN;
		udelay(2500);
#endif
                uart[unit]->brg = PIC32_BRG_BAUD (BUS_KHZ * 1000, UART6_BAUD);
                break;
        }

		uart[unit]->sta = 0;
		uart[unit]->mode = PIC32_UMODE_PDSEL_8NPAR |	/* 8-bit data, no parity */
		    PIC32_UMODE_ON;		/* UART Enable */
		uart[unit]->staset = PIC32_USTA_URXEN |	/* Receiver Enable */
		      PIC32_USTA_UTXEN;		/* Transmit Enable */
	}
}

int uartopen(dev_t dev, int flag, int mode)
{
	register struct uartreg *reg;
	register struct tty *tp;
	register int unit = minor(dev);

	if (unit >= NUART)
		return (ENXIO);

#ifndef UART1_ENABLED
        if (unit==0)
            return ENODEV;
#endif
#ifndef UART2_ENABLED
        if (unit==1)
            return ENODEV;
#endif
#ifndef UART3_ENABLED
        if (unit==2)
            return ENODEV;
#endif
#ifndef UART4_ENABLED
        if (unit==3)
            return ENODEV;
#endif
#ifndef UART5_ENABLED
        if (unit==4)
            return ENODEV;
#endif
#ifndef UART6_ENABLED
        if (unit==5)
            return ENODEV;
#endif

	if ((SD0_PORT == 2) && (unit==2 || unit==5))
		return (ENODEV);
	if ((SD0_PORT == 3) && (unit==0 || unit==3))
		return (ENODEV);
	if ((SD0_PORT == 4) && (unit==1 || unit==4))
		return (ENODEV);

	tp = &uartttys[unit];
	if (! tp->t_addr) {
	        switch (unit) {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                        tp->t_addr = (caddr_t) uart[unit];
                        break;
		default:
                        return (ENXIO);
		}
        }
        reg = (struct uartreg*) tp->t_addr;
	tp->t_oproc = uartstart;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		if (tp->t_ispeed == 0) {
            switch(unit)
            {
                case 0:
                    tp->t_ispeed = BBAUD(UART1_BAUD);
                    tp->t_ospeed = BBAUD(UART1_BAUD);
                    break;
                case 1:
                    tp->t_ispeed = BBAUD(UART2_BAUD);
                    tp->t_ospeed = BBAUD(UART2_BAUD);
                    break;
                case 2:
                    tp->t_ispeed = BBAUD(UART3_BAUD);
                    tp->t_ospeed = BBAUD(UART3_BAUD);
                    break;
                case 3:
                    tp->t_ispeed = BBAUD(UART4_BAUD);
                    tp->t_ospeed = BBAUD(UART4_BAUD);
                    break;
                case 4:
                    tp->t_ispeed = BBAUD(UART5_BAUD);
                    tp->t_ospeed = BBAUD(UART5_BAUD);
                    break;
                case 5:
                    tp->t_ispeed = BBAUD(UART6_BAUD);
                    tp->t_ospeed = BBAUD(UART6_BAUD);
                    break;
            }
		}
		ttychars(tp);
		tp->t_state = TS_ISOPEN | TS_CARR_ON;
		tp->t_flags = ECHO | XTABS | CRMOD | CRTBS | CRTERA | CTLECH | CRTKIL;
	}
	if ((tp->t_state & TS_XCLUDE) && u.u_uid != 0)
		return (EBUSY);

	reg->sta = 0;
	reg->brg = PIC32_BRG_BAUD (BUS_KHZ * 1000, speed_bps [tp->t_ospeed]);
	reg->mode = PIC32_UMODE_PDSEL_8NPAR |
                    PIC32_UMODE_ON;
	reg->staset = PIC32_USTA_URXEN | PIC32_USTA_UTXEN;

	/* Enable receive interrupt. */
	if (uirq[unit].rx < 32)
	{
		IECSET(0) = 1 << uirq[unit].rx;
	} else if (uirq[unit].rx < 64)
	{
		IECSET(1) = 1 << (uirq[unit].rx-32);
	} else {
		IECSET(2) = 1 << (uirq[unit].rx-64);
	}
	return ttyopen(dev, tp);
}

/*ARGSUSED*/
int
uartclose (dev, flag, mode)
	dev_t dev;
{
	register int unit = minor(dev);
	register struct tty *tp = &uartttys[unit];
#ifndef UART1_ENABLED
        if (unit==0)
            return ENODEV;
#endif
#ifndef UART2_ENABLED
        if (unit==1)
            return ENODEV;
#endif
#ifndef UART3_ENABLED
        if (unit==2)
            return ENODEV;
#endif
#ifndef UART4_ENABLED
        if (unit==3)
            return ENODEV;
#endif
#ifndef UART5_ENABLED
        if (unit==4)
            return ENODEV;
#endif
#ifndef UART6_ENABLED
        if (unit==5)
            return ENODEV;
#endif

	if ((SD0_PORT == 2) && (unit==2 || unit==5))
		return (ENODEV);
	if ((SD0_PORT == 3) && (unit==0 || unit==3))
		return (ENODEV);
	if ((SD0_PORT == 4) && (unit==1 || unit==4))
		return (ENODEV);

	ttyclose(tp);
	return(0);
}

/*ARGSUSED*/
int
uartread (dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register int unit = minor(dev);
	register struct tty *tp = &uartttys[unit];
#ifndef UART1_ENABLED
        if (unit==0)
            return ENODEV;
#endif
#ifndef UART2_ENABLED
        if (unit==1)
            return ENODEV;
#endif
#ifndef UART3_ENABLED
        if (unit==2)
            return ENODEV;
#endif
#ifndef UART4_ENABLED
        if (unit==3)
            return ENODEV;
#endif
#ifndef UART5_ENABLED
        if (unit==4)
            return ENODEV;
#endif
#ifndef UART6_ENABLED
        if (unit==5)
            return ENODEV;
#endif

        if ((SD0_PORT == 2) && (unit==2 || unit==5))
                return (ENODEV);
        if ((SD0_PORT == 3) && (unit==0 || unit==3))
                return (ENODEV);
        if ((SD0_PORT == 4) && (unit==1 || unit==4))
                return (ENODEV);

	return ttread(tp, uio, flag);
}

/*ARGSUSED*/
int
uartwrite (dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register int unit = minor(dev);
	register struct tty *tp = &uartttys[unit];

#ifndef UART1_ENABLED
        if (unit==0)
            return ENODEV;
#endif
#ifndef UART2_ENABLED
        if (unit==1)
            return ENODEV;
#endif
#ifndef UART3_ENABLED
        if (unit==2)
            return ENODEV;
#endif
#ifndef UART4_ENABLED
        if (unit==3)
            return ENODEV;
#endif
#ifndef UART5_ENABLED
        if (unit==4)
            return ENODEV;
#endif
#ifndef UART6_ENABLED
        if (unit==5)
            return ENODEV;
#endif
        if ((SD0_PORT == 2) && (unit==2 || unit==5))
                return (ENODEV);
        if ((SD0_PORT == 3) && (unit==0 || unit==3))
                return (ENODEV);
        if ((SD0_PORT == 4) && (unit==1 || unit==4))
                return (ENODEV);

	return ttwrite(tp, uio, flag);
}

int
uartselect (dev, rw)
	register dev_t dev;
	int rw;
{
	register int unit = minor(dev);
	register struct tty *tp = &uartttys[unit];

#ifndef UART1_ENABLED
        if (unit==0)
            return ENODEV;
#endif
#ifndef UART2_ENABLED
        if (unit==1)
            return ENODEV;
#endif
#ifndef UART3_ENABLED
        if (unit==2)
            return ENODEV;
#endif
#ifndef UART4_ENABLED
        if (unit==3)
            return ENODEV;
#endif
#ifndef UART5_ENABLED
        if (unit==4)
            return ENODEV;
#endif
#ifndef UART6_ENABLED
        if (unit==5)
            return ENODEV;
#endif
        if ((SD0_PORT == 2) && (unit==2 || unit==5))
                return (ENODEV);
        if ((SD0_PORT == 3) && (unit==0 || unit==3))
                return (ENODEV);
        if ((SD0_PORT == 4) && (unit==1 || unit==4))
                return (ENODEV);

	return (ttyselect (tp, rw));
}

/*ARGSUSED*/
int
uartioctl (dev, cmd, addr, flag)
	dev_t dev;
	register u_int cmd;
	caddr_t addr;
{
	register int unit = minor(dev);
	register struct tty *tp = &uartttys[unit];
	register int error;

#ifndef UART1_ENABLED
        if (unit==0)
            return ENODEV;
#endif
#ifndef UART2_ENABLED
        if (unit==1)
            return ENODEV;
#endif
#ifndef UART3_ENABLED
        if (unit==2)
            return ENODEV;
#endif
#ifndef UART4_ENABLED
        if (unit==3)
            return ENODEV;
#endif
#ifndef UART5_ENABLED
        if (unit==4)
            return ENODEV;
#endif
#ifndef UART6_ENABLED
        if (unit==5)
            return ENODEV;
#endif
        if ((SD0_PORT == 2) && (unit==2 || unit==5))
                return (ENODEV);
        if ((SD0_PORT == 3) && (unit==0 || unit==3))
                return (ENODEV);
        if ((SD0_PORT == 4) && (unit==1 || unit==4))
                return (ENODEV);

	error = ttioctl(tp, cmd, addr, flag);
	if (error < 0)
		error = ENOTTY;
	return (error);
}

void
uartintr (dev)
	dev_t dev;
{
	register int c;
	register int unit = minor(dev);
	register struct tty *tp = &uartttys[unit];
	register struct uartreg *reg = (struct uartreg *)tp->t_addr;

#ifndef UART1_ENABLED
        if (unit==0)
            return;
#endif
#ifndef UART2_ENABLED
        if (unit==1)
            return;
#endif
#ifndef UART3_ENABLED
        if (unit==2)
            return;
#endif
#ifndef UART4_ENABLED
        if (unit==3)
            return;
#endif
#ifndef UART5_ENABLED
        if (unit==4)
            return;
#endif
#ifndef UART6_ENABLED
        if (unit==5)
            return;
#endif
        /* Receive */
	while (reg->sta & PIC32_USTA_URXDA) {
                c = reg->rxreg;
                ttyinput(c, tp);
        }
	if (reg->sta & PIC32_USTA_OERR)
		reg->staclr = PIC32_USTA_OERR;

	if (uirq[unit].rx < 32)
	{
		IFSCLR(0) = (1 << uirq[unit].rx) | (1 << uirq[unit].er);
	} else if (uirq[unit].rx < 64)
	{
		IFSCLR(1) = (1 << (uirq[unit].rx-32)) | (1 << (uirq[unit].er-32));
	} else {
		IFSCLR(2) = (1 << (uirq[unit].rx-64)) | (1 << (uirq[unit].er-64));
	}

        /* Transmit */
        if (reg->sta & PIC32_USTA_TRMT) {
                led_control (LED_TTY, 0);

		if (uirq[unit].tx < 32)
		{
			IECCLR(0) = 1 << uirq[unit].tx;
			IFSCLR(0) = 1 << uirq[unit].tx;
		} else if (uirq[unit].tx < 64) {
			IECCLR(1) = 1 << (uirq[unit].tx - 32);
			IFSCLR(1) = 1 << (uirq[unit].tx - 32);
		} else {
			IECCLR(2) = 1 << (uirq[unit].tx - 64);
			IFSCLR(2) = 1 << (uirq[unit].tx - 64);
		}
                if (tp->t_state & TS_BUSY) {
                        tp->t_state &= ~TS_BUSY;
                        ttstart(tp);
                }
        }
}

void uartstart (register struct tty *tp)
{
	register struct uartreg *reg = (struct uartreg*) tp->t_addr;
	register int c, s;
	register int unit = minor(tp->t_dev);

#ifndef UART1_ENABLED
    if (unit==0)
        return;
#endif
#ifndef UART2_ENABLED
    if (unit==1)
        return;
#endif
#ifndef UART3_ENABLED
    if (unit==2)
        return;
#endif
#ifndef UART4_ENABLED
    if (unit==3)
        return;
#endif
#ifndef UART5_ENABLED
    if (unit==4)
        return;
#endif
#ifndef UART6_ENABLED
    if (unit==5)
        return;
#endif
	s = spltty();
	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP)) {
out:		/* Disable transmit_interrupt. */
        led_control (LED_TTY, 0);
		splx (s);
		return;
	}
	ttyowake(tp);
	if (tp->t_outq.c_cc == 0)
		goto out;
	if (reg->sta & PIC32_USTA_TRMT) {
		c = getc(&tp->t_outq);
		reg->txreg = c & 0xff;
		tp->t_state |= TS_BUSY;
	}
	/* Enable transmit interrupt. */
	if (uirq[unit].tx < 32) {
		IECSET(0) = 1 << uirq[unit].tx;
	} else if (uirq[unit].tx < 64) {
		IECSET(1) = 1 << (uirq[unit].tx - 32);
	} else {
		IECSET(2) = 1 << (uirq[unit].tx - 64);
	}
    led_control (LED_TTY, 1);
	splx (s);
}

void uartputc(dev_t dev, char c)
{
    int unit = minor(dev);
    struct tty *tp = &uartttys[unit];
    register struct uartreg *reg = uart[unit];
    register int s, timo;

#ifndef UART1_ENABLED
    if (unit==0)
        return;
#endif
#ifndef UART2_ENABLED
    if (unit==1)
        return;
#endif
#ifndef UART3_ENABLED
    if (unit==2)
        return;
#endif
#ifndef UART4_ENABLED
    if (unit==3)
        return;
#endif
#ifndef UART5_ENABLED
    if (unit==4)
        return;
#endif
#ifndef UART6_ENABLED
    if (unit==5)
        return;
#endif
    s = spltty();
again:
    /*
     * Try waiting for the console tty to come ready,
     * otherwise give up after a reasonable time.
     */
    timo = 30000;
    while ((reg->sta & PIC32_USTA_TRMT) == 0)
        if (--timo == 0)
            break;
    if (tp->t_state & TS_BUSY) {
            uartintr (0);
            goto again;
    }
    led_control (LED_TTY, 1);
    reg->txreg = c;

    timo = 30000;
    while ((reg->sta & PIC32_USTA_TRMT) == 0)
        if (--timo == 0)
            break;

    /* Clear TX interrupt. */
	if (uirq[unit].tx < 32) {
		IECCLR(0) = 1 << uirq[unit].tx;
	} else if (uirq[unit].tx < 64) {
		IECCLR(1) = 1 << (uirq[unit].tx - 32);
	} else {
		IECCLR(2) = 1 << (uirq[unit].tx - 64);
	}
    led_control(LED_TTY, 0);
    splx(s);
}


char uartgetc(dev_t dev)
{
    int unit = minor(dev);
    register struct uartreg *reg = uart[unit];
    int s, c;

#ifndef UART1_ENABLED
    if (unit == 0)
        return ENODEV;
#endif
#ifndef UART2_ENABLED
    if (unit == 1)
        return ENODEV;
#endif
#ifndef UART3_ENABLED
    if (unit == 2)
        return ENODEV;
#endif
#ifndef UART4_ENABLED
    if (unit == 3)
        return ENODEV;
#endif
#ifndef UART5_ENABLED
    if (unit == 4)
        return ENODEV;
#endif
#ifndef UART6_ENABLED
    if (unit == 5)
        return ENODEV;
#endif
    s = spltty();
    for (;;) {
        /* Wait for key pressed. */
        if (reg->sta & PIC32_USTA_URXDA) {
            c = reg->rxreg;
            break;
        }
    }
	if (uirq[unit].rx < 32) {
		IFSCLR(0) = (1 << uirq[unit].rx) | (1 << uirq[unit].er);
	} else if (uirq[unit].rx < 64) {
		IFSCLR(1) = (1 << (uirq[unit].rx-32)) | (1 << (uirq[unit].er-32));
	} else {
		IFSCLR(2) = (1 << (uirq[unit].rx-64)) | (1 << (uirq[unit].er-64));
	}
    splx(s);
    return (unsigned char) c;
}
