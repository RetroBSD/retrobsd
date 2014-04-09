#ifndef _SPI_BUS_H
#define _SPI_BUS_H

#ifdef KERNEL

struct spireg {
    volatile unsigned con;		/* Control */
    volatile unsigned conclr;
    volatile unsigned conset;
    volatile unsigned coninv;
    volatile unsigned stat;		/* Status */
    volatile unsigned statclr;
    volatile unsigned statset;
    volatile unsigned statinv;
    volatile unsigned buf;		/* Transmit and receive buffer */
    volatile unsigned unused1;
    volatile unsigned unused2;
    volatile unsigned unused3;
    volatile unsigned brg;		/* Baud rate generator */
    volatile unsigned brgclr;
    volatile unsigned brgset;
    volatile unsigned brginv;
};

struct spi_dev {
    struct spireg *bus;
    unsigned int *cs_tris;
    unsigned int cs_pin;
    unsigned int baud;
    unsigned int mode;
};

extern int spi_open(unsigned int bus, unsigned int *tris, unsigned int pin);
extern void spi_close(int dno);
extern void spi_set_cspin(int dno, unsigned int *tris, unsigned int pin);
extern void spi_select(int dno);
extern void spi_deselect(int dno);
extern void spi_set(int dno, unsigned int set);
extern void spi_clr(int dno, unsigned int set);
extern unsigned int spi_status(int dno);
extern unsigned char spi_transfer(int dno, unsigned char data);
extern void spi_bulk_write_32_be(int dno, unsigned int len, char *data);
extern void spi_bulk_write_32(int dno, unsigned int len, char *data);
extern void spi_bulk_write_16(int dno, unsigned int len, char *data);
extern void spi_bulk_write(int dno, unsigned int len, unsigned char *data);
extern void spi_bulk_read_32_be(int dno, unsigned int len, char *data);
extern void spi_bulk_read_32(int dno, unsigned int len, char *data);
extern void spi_bulk_read_16(int dno, unsigned int len, char *data);
extern void spi_bulk_read(int dno, unsigned int len, unsigned char *data);
extern void spi_bulk_rw_32_be(int dno, unsigned int len, char *data);
extern void spi_bulk_rw_32(int dno, unsigned int len, char *data);
extern void spi_bulk_rw_16(int dno, unsigned int len, char *data);
extern void spi_bulk_rw(int dno, unsigned int len, unsigned char *data);
extern void spi_brg(int dno, unsigned int baud);
extern char *spi_name(int dno);
extern char spi_csname(int dno);
extern int spi_cspin(int dno);
extern unsigned int spi_get_brg(int dno);

#endif

#endif
