#ifndef _CONF_H
#define _CONF_H
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
struct uio;
struct buf;
struct tty;

/*
 * Declaration of block device
 * switch. Each entry (row) is
 * the only link between the
 * main unix code and the driver.
 * The initialization of the
 * device switches is in the
 * file conf.c.
 */
struct bdevsw
{
    int     (*d_open) (dev_t, int, int);
    int     (*d_close) (dev_t, int, int);
    void    (*d_strategy) (struct buf*);
    daddr_t (*d_psize) (dev_t);                     /* query partition size */
    int     (*d_ioctl) (dev_t, u_int, caddr_t, int);
    int     d_flags;                                /* tape flag */
};

#define MEM_MAJOR       1

/*
 * Character device switch.
 */
struct cdevsw
{
    int     (*d_open) (dev_t, int, int);
    int     (*d_close) (dev_t, int, int);
    int     (*d_read) (dev_t, struct uio*, int);
    int     (*d_write) (dev_t, struct uio*, int);
    int     (*d_ioctl) (dev_t, u_int, caddr_t, int);
    int     (*d_stop) (struct tty*, int);
    struct tty *d_ttys;
    int     (*d_select) (dev_t, int);
    void    (*d_strategy) (struct buf*);
    char    (*r_read) (dev_t);
    void    (*r_write) (dev_t, char);
};

#define UART_MAJOR      6
#define UARTUSB_MAJOR   7
#define GPANEL_MAJOR    16

#ifdef KERNEL
#ifndef CONS_MINOR
#define CONS_MINOR      0
#endif

extern const struct bdevsw bdevsw[];
extern const struct cdevsw cdevsw[];

int rawrw (dev_t dev, struct uio *uio, int flag);
#endif

#endif
