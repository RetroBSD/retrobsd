#include <sys/param.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/spi.h>

#define NSPI    4       /* Ports SPI1...SPI4 */

static struct spireg *const spi_base[NSPI] = {
    (struct spireg*) &SPI1CON,
    (struct spireg*) &SPI2CON,
    (struct spireg*) &SPI3CON,
    (struct spireg*) &SPI4CON,
};

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
int spi_setup(struct spiio *io, int channel, int cs)
{
    unsigned *tris = 0;
    int pin = 0;

    if (channel <= 0 || channel > NSPI)
        return ENXIO;

    if (cs != 0) {
        /* Compute the port address and pin index of the chip select signal. */
        int port = (cs >> 4) - 1;
        tris = (unsigned*) (port + (struct gpioreg*) &TRISA);
        pin = cs & 15;
    }

    // Set up the device
    io->bus = spi_base[channel-1];
    io->cs_tris = tris;
    io->cs_pin = pin;
    io->baud = (BUS_KHZ / SPI_MHZ / 1000 + 1) / 2 - 1;
    io->mode = PIC32_SPICON_MSTEN | PIC32_SPICON_ON;

    if (tris) {
        // Configure the CS pin
        LAT_SET(*tris) = 1<<pin;
        TRIS_CLR(*tris) = 1<<pin;
    }
    return 0;
}

void spi_set_cspin(struct spiio *io, unsigned int *tris, unsigned int pin)
{
    // Revert the old CS pin to an input (release it)
    if (io->cs_tris) {
        // Configure the CS pin
        TRIS_SET(*io->cs_tris) = 1<<pin;
    }

    io->cs_tris = tris;
    io->cs_pin = pin;
    if (tris) {
        // Configure the CS pin
        LAT_SET(*tris) = 1<<pin;
        TRIS_CLR(*tris) = 1<<pin;
    }
}

//
// Assert the CS pin of a device.
// Not only do we set the CS pin, but before we do so we also reconfigure
// the SPI bus to the required settings for this device.
//
void spi_select(struct spiio *io)
{
    if (io->cs_tris == NULL)
        return;

    io->bus->brg = io->baud;
    io->bus->con = io->mode;

    LAT_CLR(*io->cs_tris) = 1 << io->cs_pin;
}

//
// Deassert the CS pin of a device.
//
void spi_deselect(struct spiio *io)
{
    if (io->cs_tris == NULL)
        return;

    LAT_SET(*io->cs_tris) = 1 << io->cs_pin;
}

//
// Set a mode setting or two - just updates the internal records, the
// actual mode is changed next time the CS is asserted
//
void spi_set(struct spiio *io, unsigned int set)
{
    io->mode |= set;
}

//
// Clear a mode setting or two - just updates the internal records, the
// actual mode is changed next time the CS is asserted
//
void spi_clr(struct spiio *io, unsigned int set)
{
    io->mode &= ~set;
}

//
// Return the current status of the SPI bus for the device in question
// Just returns the ->stat entry in the register set.
//
unsigned int spi_status(struct spiio *io)
{
    return io->bus->stat;
}

//
// Transfer one word of data, and return the read word of
// data.  The actual number of bits sent depends on the
// mode of the transfer.
// This is blocking, and waits for the transfer to complete
// before returning.  Times out after a certain period.
//
unsigned char spi_transfer(struct spiio *io, unsigned char data)
{
    struct spireg *reg = io->bus;
    unsigned int to = 100000;

    if (! reg)
        return 0xF1;

    reg->con = io->mode;
    reg->brg = io->baud;

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
void spi_bulk_write_32_be(struct spiio *io, unsigned int len, char *data)
{
    struct spireg *reg = io->bus;
    int *data32 = (int *)data;
    unsigned int words = len >> 2;
    unsigned int nread;
    unsigned int nwritten;

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
    reg->con = io->mode;
}

void spi_bulk_write_32(struct spiio *io, unsigned int len, char *data)
{
    struct spireg *reg = io->bus;
    int *data32 = (int *)data;
    unsigned int words = len >> 2;
    unsigned int nread;
    unsigned int nwritten;

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
    reg->con = io->mode;
}

void spi_bulk_write_16(struct spiio *io, unsigned int len, char *data)
{
    struct spireg *reg = io->bus;
    short *data16 = (short *)data;
    unsigned int words = len >> 1;
    unsigned int nread;
    unsigned int nwritten;

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
    reg->con = io->mode;
}

void spi_bulk_write(struct spiio *io, unsigned int len, unsigned char *data)
{
    unsigned char *data8 = data;
    unsigned int i;
    unsigned char out;

    for (i=0; i<len; i++) {
        out = *data8;
        spi_transfer(io, out);
        data8++;
    }
}

//
// Read a huge chunk of data as fast and as efficiently as
// possible.  Switches in to 32-bit mode regardless, and uses
// the enhanced buffer mode.
// Data should be a multiple of 32 bits.
//
void spi_bulk_read_32_be(struct spiio *io, unsigned int len, char *data)
{
    struct spireg *reg = io->bus;
    int *data32 = (int *)data;
    unsigned int words = len >> 2;
    unsigned int nread;
    unsigned int nwritten;

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
    reg->con = io->mode;
}

void spi_bulk_read_32(struct spiio *io, unsigned int len, char *data)
{
    struct spireg *reg = io->bus;
    int *data32 = (int *)data;
    unsigned int words = len >> 2;
    unsigned int nread;
    unsigned int nwritten;

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
    reg->con = io->mode;
}

void spi_bulk_read_16(struct spiio *io, unsigned int len, char *data)
{
    struct spireg *reg = io->bus;
    short *data16 = (short *)data;
    unsigned int words = len >> 1;
    unsigned int nread;
    unsigned int nwritten;

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
    reg->con = io->mode;
}

void spi_bulk_read(struct spiio *io, unsigned int len, unsigned char *data)
{
    unsigned char *data8 = data;
    unsigned int i;
    unsigned char in,out;

    for (i=0; i<len; i++) {
        out = 0xFF;
        in = spi_transfer(io, out);
        *data8 = in;
        data8++;
    }
}

void spi_bulk_rw_32_be(struct spiio *io, unsigned int len, char *data)
{
    struct spireg *reg = io->bus;
    int *read32 = (int *)data;
    int *write32 = (int *)data;
    unsigned int words = len >> 2;
    unsigned int nread;
    unsigned int nwritten;

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
    reg->con = io->mode;
}

void spi_bulk_rw_32(struct spiio *io, unsigned int len, char *data)
{
    struct spireg *reg = io->bus;
    int *read32 = (int *)data;
    int *write32 = (int *)data;
    unsigned int words = len >> 2;
    unsigned int nread;
    unsigned int nwritten;

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
    reg->con = io->mode;
}

void spi_bulk_rw_16(struct spiio *io, unsigned int len, char *data)
{
    struct spireg *reg = io->bus;
    short *read16 = (short *)data;
    short *write16 = (short *)data;
    unsigned int words = len >> 1;
    unsigned int nread;
    unsigned int nwritten;

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
    reg->con = io->mode;
}

void spi_bulk_rw(struct spiio *io, unsigned int len, unsigned char *data)
{
    unsigned char *data8 = data;
    unsigned int i;
    unsigned char in,out;

    for (i=0; i<len; i++) {
        out = *data8;
        in = spi_transfer(io, out);
        *data8 = in;
        data8++;
    }
}

//
// Set the SPI baud rate for a device (in KHz)
//
void spi_brg(struct spiio *io, unsigned int baud)
{
    io->baud = (BUS_KHZ / baud + 1) / 2 - 1;
}

//
// Return the name of the SPI bus for a device
//
char *spi_name(struct spiio *io)
{
    if (io->bus == spi_base[0])
        return "SPI1";

    if (io->bus == spi_base[1])
        return "SPI2";

    if (io->bus == spi_base[2])
        return "SPI3";

    if (io->bus == spi_base[3])
        return "SPI4";

    return "SPI?";
}

//
// Return the port name of the CS pin for a device
//
char spi_csname(struct spiio *io)
{
    switch ((unsigned)io->cs_tris) {
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

int spi_cspin(struct spiio *io)
{
    return io->cs_pin;
}

unsigned int spi_get_brg(struct spiio *io)
{
    return BUS_KHZ / (io->baud + 1) / 2;
}
