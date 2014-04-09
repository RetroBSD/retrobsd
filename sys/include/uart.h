#ifndef _UART_H
#define _UART_H

#include "tty.h"

#define NUART 6
#define UART_MAJOR 12

#ifdef KERNEL
#include "conf.h"

struct uart_irq {
	int	er;
	int 	rx;
	int	tx;
};

extern const struct uart_irq uirq[NUART];

/*
 * PIC32 UART registers.
 */
struct uartreg {
        volatile unsigned mode;		/* Mode */
        volatile unsigned modeclr;
        volatile unsigned modeset;
        volatile unsigned modeinv;
        volatile unsigned sta;		/* Status and control */
        volatile unsigned staclr;
        volatile unsigned staset;
        volatile unsigned stainv;
        volatile unsigned txreg;	/* Transmit */
        volatile unsigned unused1;
        volatile unsigned unused2;
        volatile unsigned unused3;
        volatile unsigned rxreg;	/* Receive */
        volatile unsigned unused4;
        volatile unsigned unused5;
        volatile unsigned unused6;
        volatile unsigned brg;		/* Baud rate */
        volatile unsigned brgclr;
        volatile unsigned brgset;
        volatile unsigned brginv;
};

extern struct tty uartttys[NUART];
extern struct uartreg *uart[NUART];
extern void uartinit();
extern int uartopen (dev_t dev, int flag, int mode);
extern int uartclose (dev_t dev, int flag, int mode);
extern int uartread (dev_t dev, struct uio * uio, int flag);
extern int uartwrite (dev_t dev, struct uio *uio, int flag);
extern int uartselect (register dev_t dev, int rw);
extern int uartioctl (dev_t dev, register u_int cmd, caddr_t addr, int flag);
extern void uartintr(dev_t dev);
extern void uartstart (register struct tty *tp);
extern void uartputc(dev_t dev, char c);
extern char uartgetc(dev_t dev);

extern const struct devspec uartdevs[];
extern unsigned int uart_major;

#endif

#endif
