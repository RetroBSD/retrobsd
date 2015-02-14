#include "param.h"
#include "conf.h"
#include "user.h"
#include "ioctl.h"
#include "systm.h"
#include "uio.h"
#include "spi_bus.h"

#define NSPI    4       /* Ports SPI1...SPI4 */

#define MAXSPIDEV 20

struct spi_dev spi_devices[MAXSPIDEV];

//
// Default SPI bus speed
//
#ifndef SPI_MHZ
#define SPI_MHZ 10
#endif

//
// Open an SPI device in default mode.  Use further function calls to
// set baud rate, clock phase, etc.
// Returns an integer for the number of the device (ala fd).
// Returns -1 if no devices are available.
//
int spi_open(unsigned int bus, unsigned int *tris, unsigned int pin)
{
    int dno;
    struct spi_dev *dev;

    // Find a free device
    for (dno=0; dno<MAXSPIDEV && spi_devices[dno].bus != NULL; dno++);

    // or return if not found
    if (dno == MAXSPIDEV)
        return -1;
    dev = &spi_devices[dno];

    // Set up the device
    switch (bus) {
    case 1:
        dev->bus = (struct spireg *)&SPI1CON;
        break;
    case 2:
        dev->bus = (struct spireg *)&SPI2CON;
        break;
    case 3:
        dev->bus = (struct spireg *)&SPI3CON;
        break;
    case 4:
        dev->bus = (struct spireg *)&SPI4CON;
        break;
    default:
        return -1;
    }
    dev->cs_tris = tris;
    dev->cs_pin = pin;
    dev->baud = (BUS_KHZ / SPI_MHZ / 1000 + 1) / 2 - 1;
    dev->mode = PIC32_SPICON_MSTEN | PIC32_SPICON_ON;

    if (tris) {
        // Configure the CS pin
        LAT_SET(*tris) = 1<<pin;
        TRIS_CLR(*tris) = 1<<pin;
    }
    // return the ID of the device.
    return dno;
}

void spi_set_cspin(int dno, unsigned int *tris, unsigned int pin)
{
    struct spi_dev *dev;

    if (dno >= MAXSPIDEV)
        return;

    dev = &spi_devices[dno];
    if (! dev->bus)
        return;

    // Revert the old CS pin to an input (release it)
    if (dev->cs_tris) {
        // Configure the CS pin
        TRIS_SET(*dev->cs_tris) = 1<<pin;
    }

    dev->cs_tris = tris;
    dev->cs_pin = pin;
    if (tris) {
        // Configure the CS pin
        LAT_SET(*tris) = 1<<pin;
        TRIS_CLR(*tris) = 1<<pin;
    }
}

//
// Close an SPI device
// Free up the device entry, and turn off the CS pin (set it to input)
//
void spi_close(int dno)
{
    struct spi_dev *dev;

    if (dno >= MAXSPIDEV)
        return;

    dev = &spi_devices[dno];
    if (! dev->bus)
        return;

    if (dev->cs_tris != NULL) {
        // Revert the CS pin to input.
        TRIS_CLR(*dev->cs_tris) = 1 << dev->cs_pin;
    }
    dev->cs_tris = NULL;

    // Disable the device (remove the bus pointer)
    dev->bus = NULL;
}

//
// Assert the CS pin of a device.
// Not only do we set the CS pin, but before we do so we also reconfigure
// the SPI bus to the required settings for this device.
//
void spi_select(int dno)
{
    struct spi_dev *dev;

    if (dno >= MAXSPIDEV)
        return;

    dev = &spi_devices[dno];
    if (! dev->bus)
        return;

    if (dev->cs_tris == NULL)
        return;

    dev->bus->brg = dev->baud;
    dev->bus->con = dev->mode;

    LAT_CLR(*dev->cs_tris) = 1 << dev->cs_pin;
}

//
// Deassert the CS pin of a device.
//
void spi_deselect(int dno)
{
    struct spi_dev *dev;

    if (dno >= MAXSPIDEV)
        return;

    dev = &spi_devices[dno];
    if (! dev->bus)
        return;

    if (dev->cs_tris == NULL)
        return;

    LAT_SET(*dev->cs_tris) = 1 << dev->cs_pin;
}

//
// Set a mode setting or two - just updates the internal records, the
// actual mode is changed next time the CS is asserted
//
void spi_set(int dno, unsigned int set)
{
    struct spi_dev *dev;

    if (dno >= MAXSPIDEV)
        return;

    dev = &spi_devices[dno];
    if (! dev->bus)
        return;

    dev->mode |= set;
}

//
// Clear a mode setting or two - just updates the internal records, the
// actual mode is changed next time the CS is asserted
//
void spi_clr(int dno, unsigned int set)
{
    struct spi_dev *dev;

    if (dno >= MAXSPIDEV)
        return;

    dev = &spi_devices[dno];
    if (! dev->bus)
        return;

    dev->mode &= ~set;
}

//
// Return the current status of the SPI bus for the device in question
// Just returns the ->stat entry in the register set.
//
unsigned int spi_status(int dno)
{
    struct spi_dev *dev;

    if (dno >= MAXSPIDEV)
        return 0;

    dev = &spi_devices[dno];
    if (! dev->bus)
        return 0;

    return dev->bus->stat;
}

//
// Transfer one word of data, and return the read word of
// data.  The actual number of bits sent depends on the
// mode of the transfer.
// This is blocking, and waits for the transfer to complete
// before returning.  Times out after a certain period.
//
unsigned char spi_transfer(int dno, unsigned char data)
{
    struct spi_dev *dev;
    struct spireg *reg;
    unsigned int to = 100000;

    if (dno >= MAXSPIDEV)
        return 0xF0;

    dev = &spi_devices[dno];
    reg = dev->bus;
    if (! reg)
        return 0xF1;

    reg->con = dev->mode;
    reg->brg = dev->baud;

    reg->buf = data;
    while ((--to > 0) && ! (reg->stat & PIC32_SPISTAT_SPIRBF))
        asm volatile ("nop");

    if (to  == 0)
        return 0xF2;

    return reg->buf;
}

//
// Write a huge chunk of data as fast and as efficiently as
// possible.  Switches in to 32-bit mode regardless, and uses
// the enhanced buffer mode.
// Data should be a multiple of 32 bits.
//
void spi_bulk_write_32_be(int dno, unsigned int len, char *data)
{
    struct spi_dev *dev;
    struct spireg *reg;
    int *data32 = (int *)data;
    unsigned int words = len >> 2;
    unsigned int nread;
    unsigned int nwritten;

    if (dno >= MAXSPIDEV)
        return;

    dev = &spi_devices[dno];
    reg = dev->bus;
    if (! reg)
        return;

    nread = 0;
    nwritten = words;

    reg->conset = PIC32_SPICON_MODE32 | PIC32_SPICON_ENHBUF;
    while (nread < words)
    {
        if (nwritten > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF))
        {
            reg->buf = mips_bswap(*data32++);
            nwritten--;
        }

        if (! (reg->stat & PIC32_SPISTAT_SPIRBE))
        {
            (void) reg->buf;
            nread++;
        }
    }
    reg->con = dev->mode;
}

void spi_bulk_write_32(int dno, unsigned int len, char *data)
{
    struct spi_dev *dev;
    struct spireg *reg;
    int *data32 = (int *)data;
    unsigned int words = len >> 2;
    unsigned int nread;
    unsigned int nwritten;

    if (dno >= MAXSPIDEV)
        return;

    dev = &spi_devices[dno];
    reg = dev->bus;
    if (! reg)
        return;

    nread = 0;
    nwritten = words;

    reg->conset = PIC32_SPICON_MODE32 | PIC32_SPICON_ENHBUF;
    while (nread < words)
    {
        if (nwritten > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF))
        {
            reg->buf = *data32++;
            nwritten--;
        }

        if (! (reg->stat & PIC32_SPISTAT_SPIRBE))
        {
            (void) reg->buf;
            nread++;
        }
    }
    reg->con = dev->mode;
}

void spi_bulk_write_16(int dno, unsigned int len, char *data)
{
    struct spi_dev *dev;
    struct spireg *reg;
    short *data16 = (short *)data;
    unsigned int words = len >> 1;
    unsigned int nread;
    unsigned int nwritten;

    if (dno >= MAXSPIDEV)
        return;

    dev = &spi_devices[dno];
    reg = dev->bus;
    if (! reg)
        return;

    nread = 0;
    nwritten = words;

    reg->conset = PIC32_SPICON_MODE16 | PIC32_SPICON_ENHBUF;
    while (nread < words)
    {
        if (nwritten > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF))
        {
            reg->buf = *data16++;
            nwritten--;
        }

        if (! (reg->stat & PIC32_SPISTAT_SPIRBE))
        {
            (void) reg->buf;
            nread++;
        }
    }
    reg->con = dev->mode;
}

void spi_bulk_write(int dno, unsigned int len, unsigned char *data)
{
    struct spi_dev *dev;
    unsigned char *data8 = data;
    unsigned int i;
    unsigned char out;

    if (dno >= MAXSPIDEV)
        return;

    dev = &spi_devices[dno];
    if (! dev->bus)
        return;

    for (i=0; i<len; i++) {
        out = *data8;
        spi_transfer(dno, out);
        data8++;
    }
}

//
// Read a huge chunk of data as fast and as efficiently as
// possible.  Switches in to 32-bit mode regardless, and uses
// the enhanced buffer mode.
// Data should be a multiple of 32 bits.
//
void spi_bulk_read_32_be(int dno, unsigned int len, char *data)
{
    struct spi_dev *dev;
    struct spireg *reg;
    int *data32 = (int *)data;
    unsigned int words = len >> 2;
    unsigned int nread;
    unsigned int nwritten;

    if (dno >= MAXSPIDEV)
        return;

    dev = &spi_devices[dno];
    reg = dev->bus;
    if (! reg)
        return;

    nread = 0;
    nwritten = words;

    reg->conset = PIC32_SPICON_MODE32 | PIC32_SPICON_ENHBUF;
    while (nread < words)
    {
        if (nwritten > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF))
        {
            reg->buf = ~0;
            nwritten--;
        }

        if (! (reg->stat & PIC32_SPISTAT_SPIRBE))
        {
            *data32++ = mips_bswap(reg->buf);
            nread++;
        }
    }
    reg->con = dev->mode;
}

void spi_bulk_read_32(int dno, unsigned int len, char *data)
{
    struct spi_dev *dev;
    struct spireg *reg;
    int *data32 = (int *)data;
    unsigned int words = len >> 2;
    unsigned int nread;
    unsigned int nwritten;

    if (dno >= MAXSPIDEV)
        return;

    dev = &spi_devices[dno];
    reg = dev->bus;
    if (! reg)
        return;

    nread = 0;
    nwritten = words;

    reg->conset = PIC32_SPICON_MODE32 | PIC32_SPICON_ENHBUF;
    while (nread < words)
    {
        if (nwritten > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF))
        {
            reg->buf = ~0;
            nwritten--;
        }

        if (! (reg->stat & PIC32_SPISTAT_SPIRBE))
        {
            *data32++ = reg->buf;
            nread++;
        }
    }
    reg->con = dev->mode;
}

void spi_bulk_read_16(int dno, unsigned int len, char *data)
{
    struct spi_dev *dev;
    struct spireg *reg;
    short *data16 = (short *)data;
    unsigned int words = len >> 1;
    unsigned int nread;
    unsigned int nwritten;

    if (dno >= MAXSPIDEV)
        return;

    dev = &spi_devices[dno];
    reg = dev->bus;
    if (! reg)
        return;

    nread = 0;
    nwritten = words;

    reg->conset = PIC32_SPICON_MODE16 | PIC32_SPICON_ENHBUF;
    while (nread < words)
    {
        if (nwritten > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF))
        {
            reg->buf = ~0;
            nwritten--;
        }

        if (! (reg->stat & PIC32_SPISTAT_SPIRBE))
        {
            *data16++ = mips_bswap(reg->buf);
            nread++;
        }
    }
    reg->con = dev->mode;
}

void spi_bulk_read(int dno, unsigned int len, unsigned char *data)
{
    struct spi_dev *dev;
    unsigned char *data8 = data;
    unsigned int i;
    unsigned char in,out;

    if (dno >= MAXSPIDEV)
        return;

    dev = &spi_devices[dno];
    if (! dev->bus)
        return;

    for (i=0; i<len; i++) {
        out = 0xFF;
        in = spi_transfer(dno, out);
        *data8 = in;
        data8++;
    }
}

void spi_bulk_rw_32_be(int dno, unsigned int len, char *data)
{
    struct spi_dev *dev;
    struct spireg *reg;
    int *read32 = (int *)data;
    int *write32 = (int *)data;
    unsigned int words = len >> 2;
    unsigned int nread;
    unsigned int nwritten;

    if (dno >= MAXSPIDEV)
        return;

    dev = &spi_devices[dno];
    reg = dev->bus;
    if (! reg)
        return;

    nread = 0;
    nwritten = words;

    reg->conset = PIC32_SPICON_MODE32 | PIC32_SPICON_ENHBUF;
    while (nread < words)
    {
        if (nwritten > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF))
        {
            reg->buf = *write32++;
            nwritten--;
        }

        if (! (reg->stat & PIC32_SPISTAT_SPIRBE))
        {
            *read32++ = mips_bswap(reg->buf);
            nread++;
        }
    }
    reg->con = dev->mode;
}

void spi_bulk_rw_32(int dno, unsigned int len, char *data)
{
    struct spi_dev *dev;
    struct spireg *reg;
    int *read32 = (int *)data;
    int *write32 = (int *)data;
    unsigned int words = len >> 2;
    unsigned int nread;
    unsigned int nwritten;

    if (dno >= MAXSPIDEV)
        return;

    dev = &spi_devices[dno];
    reg = dev->bus;
    if (! reg)
        return;

    nread = 0;
    nwritten = words;

    reg->conset = PIC32_SPICON_MODE32 | PIC32_SPICON_ENHBUF;
    while (nread < words)
    {
        if (nwritten > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF))
        {
            reg->buf = *write32++;
            nwritten--;
        }

        if (! (reg->stat & PIC32_SPISTAT_SPIRBE))
        {
            *read32++ = reg->buf;
            nread++;
        }
    }
    reg->con = dev->mode;
}

void spi_bulk_rw_16(int dno, unsigned int len, char *data)
{
    struct spi_dev *dev;
    struct spireg *reg;
    short *read16 = (short *)data;
    short *write16 = (short *)data;
    unsigned int words = len >> 1;
    unsigned int nread;
    unsigned int nwritten;

    if (dno >= MAXSPIDEV)
        return;

    dev = &spi_devices[dno];
    reg = dev->bus;
    if (! reg)
        return;

    nread = 0;
    nwritten = words;

    reg->conset = PIC32_SPICON_MODE16 | PIC32_SPICON_ENHBUF;
    while (nread < words)
    {
        if (nwritten > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF))
        {
            reg->buf = *write16++;
            nwritten--;
        }

        if (! (reg->stat & PIC32_SPISTAT_SPIRBE))
        {
            *read16++ = mips_bswap(reg->buf);
            nread++;
        }
    }
    reg->con = dev->mode;
}

void spi_bulk_rw(int dno, unsigned int len, unsigned char *data)
{
    struct spi_dev *dev;
    unsigned char *data8 = data;
    unsigned int i;
    unsigned char in,out;

    if (dno >= MAXSPIDEV)
        return;

    dev = &spi_devices[dno];
    if (! dev->bus)
        return;

    for (i=0; i<len; i++) {
        out = *data8;
        in = spi_transfer(dno, out);
        *data8 = in;
        data8++;
    }
}

//
// Set the SPI baud rate for a device (in KHz)
//
void spi_brg(int dno, unsigned int baud)
{
    struct spi_dev *dev;

    if (dno >= MAXSPIDEV)
        return;

    dev = &spi_devices[dno];
    if (! dev->bus)
        return;

    dev->baud = (BUS_KHZ / baud + 1) / 2 - 1;
}

//
// Return the name of the SPI bus for a device
//
char *spi_name(int dno)
{
    struct spi_dev *dev;

    if (dno >= MAXSPIDEV)
        return "SPI?";

    dev = &spi_devices[dno];
    if (! dev->bus)
        return "NO SPI BUS";

    if (dev->bus == (struct spireg *)&SPI1CON)
        return "SPI1";

    if (dev->bus == (struct spireg *)&SPI2CON)
        return "SPI2";

    if (dev->bus == (struct spireg *)&SPI3CON)
        return "SPI3";

    if (dev->bus == (struct spireg *)&SPI4CON)
        return "SPI4";

    return "SPI?";
}

//
// Return the port name of the CS pin for a device
//
char spi_csname(int dno)
{
    struct spi_dev *dev;

    if (dno >= MAXSPIDEV)
        return '?';

    dev = &spi_devices[dno];
    if (! dev->bus)
        return '?';

    switch ((unsigned)dev->cs_tris) {
    case (unsigned)&TRISA: return 'A';
    case (unsigned)&TRISB: return 'B';
    case (unsigned)&TRISC: return 'C';
    case (unsigned)&TRISD: return 'D';
    case (unsigned)&TRISE: return 'E';
    case (unsigned)&TRISF: return 'F';
    case (unsigned)&TRISG: return 'G';
    }
    return '?';
}

int spi_cspin(int dno)
{
    struct spi_dev *dev;

    if (dno >= MAXSPIDEV)
        return '?';

    dev = &spi_devices[dno];
    if (! dev->bus)
        return '?';

    return dev->cs_pin;
}

unsigned int spi_get_brg(int dno)
{
    struct spi_dev *dev;

    if (dno >= MAXSPIDEV)
        return 0;

    dev = &spi_devices[dno];
    if (! dev->bus)
        return 0;

    return BUS_KHZ / (dev->baud + 1) / 2;
}
