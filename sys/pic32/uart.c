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
#include <sys/kconfig.h>
#include <machine/uart.h>

#define CONCAT(x,y) x ## y
#define BBAUD(x) CONCAT(B,x)

#ifndef UART_BAUD
#define UART_BAUD 115200
#endif

/*
 * PIC32 UART registers.
 */
struct uartreg {
    volatile unsigned mode;     /* Mode */
    volatile unsigned modeclr;
    volatile unsigned modeset;
    volatile unsigned modeinv;
    volatile unsigned sta;      /* Status and control */
    volatile unsigned staclr;
    volatile unsigned staset;
    volatile unsigned stainv;
    volatile unsigned txreg;    /* Transmit */
    volatile unsigned unused1;
    volatile unsigned unused2;
    volatile unsigned unused3;
    volatile unsigned rxreg;    /* Receive */
    volatile unsigned unused4;
    volatile unsigned unused5;
    volatile unsigned unused6;
    volatile unsigned brg;      /* Baud rate */
    volatile unsigned brgclr;
    volatile unsigned brgset;
    volatile unsigned brginv;
};

static struct uartreg *const uart[NUART] = {
    (struct uartreg*) &U1MODE,
    (struct uartreg*) &U2MODE,
    (struct uartreg*) &U3MODE,
    (struct uartreg*) &U4MODE,
    (struct uartreg*) &U5MODE,
    (struct uartreg*) &U6MODE
};

/*
 * UART interrupt numbers.
 */
struct uart_irq {
    int     er;
    int     rx;
    int     tx;
};

static const struct uart_irq uirq[NUART] = {
    { PIC32_IRQ_U1E, PIC32_IRQ_U1RX, PIC32_IRQ_U1TX },
    { PIC32_IRQ_U2E, PIC32_IRQ_U2RX, PIC32_IRQ_U2TX },
    { PIC32_IRQ_U3E, PIC32_IRQ_U3RX, PIC32_IRQ_U3TX },
    { PIC32_IRQ_U4E, PIC32_IRQ_U4RX, PIC32_IRQ_U4TX },
    { PIC32_IRQ_U5E, PIC32_IRQ_U5RX, PIC32_IRQ_U5TX },
    { PIC32_IRQ_U6E, PIC32_IRQ_U6RX, PIC32_IRQ_U6TX },
};

struct tty uartttys[NUART];

static unsigned speed_bps [NSPEEDS] = {
    0,       50,      75,      150,     200,    300,     600,     1200,
    1800,    2400,    4800,    9600,    19200,  38400,   57600,   115200,
    230400,  460800,  500000,  576000,  921600, 1000000, 1152000, 1500000,
    2000000, 2500000, 3000000, 3500000, 4000000
};

void cnstart (struct tty *tp);

/*
 * Setup UART registers.
 * Compute the divisor for 115.2 kbaud.
 */
void uartinit(int unit)
{
    register struct uartreg *reg;

    if (unit >= NUART)
        return;

    switch(unit) {
    case 0:
#ifdef UART1_ENA_PORT
        /* Enable UART1 phy - pin is assumed to be active low */
        TRIS_CLR(UART1_ENA_PORT) = 1 << UART1_ENA_PIN;
        LAT_CLR(UART1_ENA_PORT) = 1 << UART1_ENA_PIN;
        udelay(2500);
#endif
        break;
    case 1:
#ifdef UART2_ENA_PORT
        /* Enable UART2 phy - pin is assumed to be active low */
        TRIS_CLR(UART2_ENA_PORT) = 1 << UART2_ENA_PIN;
        LAT_CLR(UART2_ENA_PORT) = 1 << UART2_ENA_PIN;
        udelay(2500);
#endif
        break;
    case 2:
#ifdef UART3_ENA_PORT
        /* Enable UART3 phy - pin is assumed to be active low */
        TRIS_CLR(UART3_ENA_PORT) = 1 << UART3_ENA_PIN;
        LAT_CLR(UART3_ENA_PORT) = 1 << UART3_ENA_PIN;
        udelay(2500);
#endif
        break;
    case 3:
#ifdef UART4_ENA_PORT
        /* Enable UART4 phy - pin is assumed to be active low */
        TRIS_CLR(UART4_ENA_PORT) = 1 << UART4_ENA_PIN;
        LAT_CLR(UART4_ENA_PORT) = 1 << UART4_ENA_PIN;
        udelay(2500);
#endif
        break;
    case 4:
#ifdef UART5_ENA_PORT
        /* Enable UART5 phy - pin is assumed to be active low */
        TRIS_CLR(UART5_ENA_PORT) = 1 << UART5_ENA_PIN;
        LAT_CLR(UART5_ENA_PORT) = 1 << UART5_ENA_PIN;
        udelay(2500);
#endif
        break;
    case 5:
#ifdef UART6_ENA_PORT
        /* Enable UART6 phy - pin is assumed to be active low */
        TRIS_CLR(UART6_ENA_PORT) = 1 << UART6_ENA_PIN;
        LAT_CLR(UART6_ENA_PORT) = 1 << UART6_ENA_PIN;
        udelay(2500);
#endif
        break;
    }

    reg = uart[unit];
    reg->brg = PIC32_BRG_BAUD (BUS_KHZ * 1000, UART_BAUD);
    reg->sta = 0;
    reg->mode =
        PIC32_UMODE_PDSEL_8NPAR |   /* 8-bit data, no parity */
        PIC32_UMODE_ON;             /* UART Enable */
    reg->staset =
        PIC32_USTA_URXEN |          /* Receiver Enable */
        PIC32_USTA_UTXEN;           /* Transmit Enable */
}

int uartopen(dev_t dev, int flag, int mode)
{
    register struct uartreg *reg;
    register struct tty *tp;
    register int unit = minor(dev);

    if (unit >= NUART)
        return (ENXIO);

    tp = &uartttys[unit];
    if (! tp->t_addr)
        return (ENXIO);

    reg = (struct uartreg*) tp->t_addr;
    tp->t_oproc = uartstart;
    if ((tp->t_state & TS_ISOPEN) == 0) {
        if (tp->t_ispeed == 0) {
            tp->t_ispeed = BBAUD(UART_BAUD);
            tp->t_ospeed = BBAUD(UART_BAUD);
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
    if (uirq[unit].rx < 32) {
            IECSET(0) = 1 << uirq[unit].rx;
    } else if (uirq[unit].rx < 64) {
            IECSET(1) = 1 << (uirq[unit].rx-32);
    } else {
            IECSET(2) = 1 << (uirq[unit].rx-64);
    }
    return ttyopen(dev, tp);
}

/*ARGSUSED*/
int
uartclose (dev_t dev, int flag, int mode)
{
    register int unit = minor(dev);
    register struct tty *tp = &uartttys[unit];

    if (! tp->t_addr)
        return ENODEV;

    ttywflush(tp);
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

    if (! tp->t_addr)
        return ENODEV;

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

    if (! tp->t_addr)
        return ENODEV;

    return ttwrite(tp, uio, flag);
}

int
uartselect (dev, rw)
    register dev_t dev;
    int rw;
{
    register int unit = minor(dev);
    register struct tty *tp = &uartttys[unit];

    if (! tp->t_addr)
        return ENODEV;

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

    if (! tp->t_addr)
        return ENODEV;

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

    if (! tp->t_addr)
        return;

    /* Receive */
    while (reg->sta & PIC32_USTA_URXDA) {
        c = reg->rxreg;
        ttyinput(c, tp);
    }
    if (reg->sta & PIC32_USTA_OERR)
        reg->staclr = PIC32_USTA_OERR;

    if (uirq[unit].rx < 32) {
        IFSCLR(0) = (1 << uirq[unit].rx) | (1 << uirq[unit].er);
    } else if (uirq[unit].rx < 64) {
        IFSCLR(1) = (1 << (uirq[unit].rx-32)) | (1 << (uirq[unit].er-32));
    } else {
        IFSCLR(2) = (1 << (uirq[unit].rx-64)) | (1 << (uirq[unit].er-64));
    }

    /* Transmit */
    if (reg->sta & PIC32_USTA_TRMT) {
        led_control (LED_TTY, 0);

        if (uirq[unit].tx < 32) {
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

    if (! tp->t_addr)
        return;

    s = spltty();
    if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP)) {
out:    /* Disable transmit_interrupt. */
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
        uartintr (dev);
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

/*
 * Test to see if device is present.
 * Return true if found and initialized ok.
 */
static int
uartprobe(config)
    struct conf_device *config;
{
    int unit = config->dev_unit - 1;
    int is_console = (CONS_MAJOR == UART_MAJOR &&
                      CONS_MINOR == unit);
    int rx, tx;
    static const int rx_tab[NUART] = {
        GPIO_PIN('D',2),    /* U1RX: 64pin - RD2, 100pin - RF2 */
        GPIO_PIN('F',4),    /* U2RX */
        GPIO_PIN('G',7),    /* U3RX */
        GPIO_PIN('D',9),    /* U4RX: 64pin - RD9, 100pin - RD14 */
        GPIO_PIN('B',8),    /* U5RX: 64pin - RB8, 100pin - RF12 */
        GPIO_PIN('G',9),    /* U6RX */
    };
    static const int tx_tab[NUART] = {
        GPIO_PIN('D',3),    /* U1TX: 64pin - RD3, 100pin - RF8 */
        GPIO_PIN('F',5),    /* U2TX */
        GPIO_PIN('G',8),    /* U3TX */
        GPIO_PIN('D',1),    /* U4TX: 64pin - RD1, 100pin - RD15 */
        GPIO_PIN('B',14),   /* U5TX: 64pin - RB14, 100pin - RF13 */
        GPIO_PIN('G',6),    /* U6TX */
    };

    if (unit < 0 || unit >= NUART)
        return 0;
    rx = rx_tab[unit];
    tx = tx_tab[unit];
    if (cpu_pins > 64) {
        /* Ports UART1, UART4 and UART5 have different pin assignments
         * for 100-pin packages. */
        switch (unit + 1) {
        case 1:
            rx = GPIO_PIN('F',2);
            tx = GPIO_PIN('F',8);
            break;
        case 4:
            rx = GPIO_PIN('D',14);
            tx = GPIO_PIN('D',15);
            break;
        case 5:
            rx = GPIO_PIN('F',12);
            tx = GPIO_PIN('F',13);
            break;
        }
    }
    printf("uart%d: pins rx=R%c%d/tx=R%c%d, interrupts %u/%u/%u", unit+1,
        gpio_portname(rx), gpio_pinno(rx),
        gpio_portname(tx), gpio_pinno(tx),
        uirq[unit].er, uirq[unit].rx, uirq[unit].tx);
    if (is_console)
        printf(", console");
    printf("\n");

    /* Initialize the device. */
    uartttys[unit].t_addr = (caddr_t) uart[unit];
    if (! is_console)
        uartinit(unit);
    return 1;
}

struct driver uartdriver = {
    "uart", uartprobe,
};
