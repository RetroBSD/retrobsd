/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Ioctl definitions for SPI driver.
 */
#ifndef _SPI_H
#define _SPI_H

#include <sys/ioctl.h>

/*
 * User-level interface to the SPI port.
 */
#define SPICTL_SETMODE      _IO ('p', 0)        /* set SPI mode */
#define SPICTL_SETRATE      _IO ('p', 1)        /* set clock rate, kHz */
#define SPICTL_SETSELPIN    _IO ('p', 2)        /* set select pin */

#define SPICTL_IO8(n)       _ION('p', 3, n)     /* transfer n*8 bits */
#define SPICTL_IO16(n)      _ION('p', 4, n)     /* transfer n*16 bits */
#define SPICTL_IO32(n)      _ION('p', 5, n)     /* transfer n*32 bits */

#define SPICTL_IO8R(n)      _ION('p', 6, n)     /* transfer R n*8 bits */
#define SPICTL_IO16R(n)     _ION('p', 7, n)     /* transfer R n*16 bits */
#define SPICTL_IO32R(n)     _ION('p', 8, n)     /* transfer R n*32 bits */

#define SPICTL_IO8W(n)      _ION('p', 9, n)     /* transfer W n*8 bits */
#define SPICTL_IO16W(n)     _ION('p', 10, n)    /* transfer W n*16 bits */
#define SPICTL_IO32W(n)     _ION('p', 11, n)    /* transfer W n*32 bits */

#define SPICTL_IO32B(n)     _ION('p', 12, n)    /* transfer BE n*32 bits */
#define SPICTL_IO32RB(n)    _ION('p', 13, n)    /* transfer RBE n*32 bits */
#define SPICTL_IO32WB(n)    _ION('p', 14, n)    /* transfer WBE n*32 bits */

#ifdef KERNEL
#include "conf.h"

/*
 * Kernel-level interface to the SPI port.
 */
struct spiio {
    struct spireg   *bus;
    unsigned int    *cs_tris;
    unsigned int    cs_pin;
    unsigned int    baud;
    unsigned int    mode;
};

extern int spi_setup(struct spiio *io, int channel, int cs);
extern void spi_set_cspin(struct spiio *io, unsigned int *tris, unsigned int pin);
extern void spi_select(struct spiio *io);
extern void spi_deselect(struct spiio *io);
extern void spi_set(struct spiio *io, unsigned int set);
extern void spi_clr(struct spiio *io, unsigned int set);
extern unsigned int spi_status(struct spiio *io);
extern unsigned char spi_transfer(struct spiio *io, unsigned char data);
extern void spi_bulk_write_32_be(struct spiio *io, unsigned int len, char *data);
extern void spi_bulk_write_32(struct spiio *io, unsigned int len, char *data);
extern void spi_bulk_write_16(struct spiio *io, unsigned int len, char *data);
extern void spi_bulk_write(struct spiio *io, unsigned int len, unsigned char *data);
extern void spi_bulk_read_32_be(struct spiio *io, unsigned int len, char *data);
extern void spi_bulk_read_32(struct spiio *io, unsigned int len, char *data);
extern void spi_bulk_read_16(struct spiio *io, unsigned int len, char *data);
extern void spi_bulk_read(struct spiio *io, unsigned int len, unsigned char *data);
extern void spi_bulk_rw_32_be(struct spiio *io, unsigned int len, char *data);
extern void spi_bulk_rw_32(struct spiio *io, unsigned int len, char *data);
extern void spi_bulk_rw_16(struct spiio *io, unsigned int len, char *data);
extern void spi_bulk_rw(struct spiio *io, unsigned int len, unsigned char *data);
extern void spi_brg(struct spiio *io, unsigned int baud);
extern char *spi_name(struct spiio *io);
extern char spi_csname(struct spiio *io);
extern int spi_cspin(struct spiio *io);
extern unsigned int spi_get_brg(struct spiio *io);

/*
 * Routines of the SPI device driver.
 */
int spidev_open (dev_t dev, int flag, int mode);
int spidev_close (dev_t dev, int flag, int mode);
int spidev_read (dev_t dev, struct uio *uio, int flag);
int spidev_write (dev_t dev, struct uio *uio, int flag);
int spidev_ioctl (dev_t dev, u_int cmd, caddr_t addr, int flag);

#endif

#endif
