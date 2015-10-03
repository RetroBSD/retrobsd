/*
 * Disk driver for serial MRAM chips connected via SPI port.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/dk.h>
#include <sys/disk.h>
#include <sys/spi.h>
#include <sys/kconfig.h>
#include <machine/debug.h>
#include <machine/mrams.h>

#define MRAM_WREN       0x06
#define MRAM_WRDI       0x04
#define MRAM_RDSR       0x05
#define MRAM_WRSR       0x01
#define MRAM_READ       0x03
#define MRAM_WRITE      0x02
#define MRAM_SLEEP      0xB9
#define MRAM_WAKE       0xAB

#ifndef MRAMS_MHZ
#define MRAMS_MHZ       13
#endif

struct spiio mrams_io[MRAMS_CHIPS];

int mrams_dkindex;                      /* disk index for statistics */

/*
 * Size of RAM disk.
 */
#define MRAMS_TOTAL_KBYTES  (MRAMS_CHIPS * MRAMS_CHIPSIZE)

#define MRBSIZE         1024
#define MRBLOG2         10

unsigned int mr_read_block(unsigned int chip, unsigned int address, unsigned int length, char *data)
{
    register unsigned int cs = 0;
    struct spiio *io = &mrams_io[chip];

    switch (chip) {
    case 0:
        #ifdef MRAMS_LED0_PORT
        TRIS_CLR(MRAMS_LED0_PORT) = 1<<MRAMS_LED0_PIN;
        LAT_SET(MRAMS_LED0_PORT) = 1<<MRAMS_LED0_PIN;
        #endif
        break;
    case 1:
        #ifdef MRAMS_LED1_PORT
        TRIS_CLR(MRAMS_LED1_PORT) = 1<<MRAMS_LED1_PIN;
        LAT_SET(MRAMS_LED1_PORT) = 1<<MRAMS_LED1_PIN;
        #endif
        break;
    case 2:
        #ifdef MRAMS_LED2_PORT
        TRIS_CLR(MRAMS_LED2_PORT) = 1<<MRAMS_LED2_PIN;
        LAT_SET(MRAMS_LED2_PORT) = 1<<MRAMS_LED2_PIN;
        #endif
        break;
    case 3:
        #ifdef MRAMS_LED3_PORT
        TRIS_CLR(MRAMS_LED3_PORT) = 1<<MRAMS_LED3_PIN;
        LAT_SET(MRAMS_LED3_PORT) = 1<<MRAMS_LED3_PIN;
        #endif
        break;
    }

    spi_select(io);
    spi_transfer(io, MRAM_READ);
    spi_transfer(io, address>>16);
    spi_transfer(io, address>>8);
    spi_transfer(io, address);

    // If the length is a multiple of 32 bits, then do a 32 bit transfer
#if 0
    if ((length & 3) == 0)
        spi_bulk_read_32(io, length, data);
    else if ((length & 1) == 0)
        spi_bulk_read_16(io, length, data);
    else
#endif
    spi_bulk_read(io, length, (unsigned char *)data);
    spi_deselect(io);

    switch (chip) {
    case 0:
        #ifdef MRAMS_LED0_PORT
        LAT_CLR(MRAMS_LED0_PORT) = 1<<MRAMS_LED0_PIN;
        #endif
        break;
    case 1:
        #ifdef MRAMS_LED1_PORT
        LAT_CLR(MRAMS_LED1_PORT) = 1<<MRAMS_LED1_PIN;
        #endif
        break;
    case 2:
        #ifdef MRAMS_LED2_PORT
        LAT_CLR(MRAMS_LED2_PORT) = 1<<MRAMS_LED2_PIN;
        #endif
        break;
    case 3:
        #ifdef MRAMS_LED3_PORT
        LAT_CLR(MRAMS_LED3_PORT) = 1<<MRAMS_LED3_PIN;
        #endif
        break;
    }
    return cs;
}

int mrams_read(unsigned int offset, char *data, unsigned int bcount)
{
    register unsigned int chip;
    register unsigned int toread;
    register unsigned int address;
    register unsigned int pass = 0;

    while (bcount > 0) {
        pass++;
        toread = bcount;
        if (toread > MRBSIZE)
            toread = MRBSIZE;

        chip = offset / MRAMS_CHIPSIZE;

        address = (offset<<10) - (chip * (MRAMS_CHIPSIZE*1024));

        if (chip >= MRAMS_CHIPS) {
            printf("!!!EIO\n");
            return EIO;
        }
        mr_read_block(chip, address, toread, data);
        bcount -= toread;
        offset += (toread>>MRBLOG2);
        data += toread;
    }
    return 1;
}

unsigned int mr_write_block(unsigned int chip, unsigned int address, unsigned int length, char *data)
{
    struct spiio *io = &mrams_io[chip];
    register unsigned int cs = 0;
    char blank __attribute__((unused));

    switch (chip) {
    case 0:
        #ifdef MRAMS_LED0_PORT
        TRIS_CLR(MRAMS_LED0_PORT) = 1<<MRAMS_LED0_PIN;
        LAT_SET(MRAMS_LED0_PORT) = 1<<MRAMS_LED0_PIN;
        #endif
        break;
    case 1:
        #ifdef MRAMS_LED1_PORT
        TRIS_CLR(MRAMS_LED1_PORT) = 1<<MRAMS_LED1_PIN;
        LAT_SET(MRAMS_LED1_PORT) = 1<<MRAMS_LED1_PIN;
        #endif
        break;
    case 2:
        #ifdef MRAMS_LED2_PORT
        TRIS_CLR(MRAMS_LED2_PORT) = 1<<MRAMS_LED2_PIN;
        LAT_SET(MRAMS_LED2_PORT) = 1<<MRAMS_LED2_PIN;
        #endif
        break;
    case 3:
        #ifdef MRAMS_LED3_PORT
        TRIS_CLR(MRAMS_LED3_PORT) = 1<<MRAMS_LED3_PIN;
        LAT_SET(MRAMS_LED3_PORT) = 1<<MRAMS_LED3_PIN;
        #endif
        break;
    }

    spi_select(io);
    spi_transfer(io, MRAM_WRITE);
    spi_transfer(io, address>>16);
    spi_transfer(io, address>>8);
    spi_transfer(io, address);

#if 0
    if ((length & 3) == 0)
        spi_bulk_write_32(io, length, data);
    else if ((length & 1) == 0)
        spi_bulk_write_16(io, length, data);
    else
#endif
    spi_bulk_write(io, length, (unsigned char *)data);
    spi_deselect(io);

    switch (chip) {
    case 0:
        #ifdef MRAMS_LED0_PORT
        LAT_CLR(MRAMS_LED0_PORT) = 1<<MRAMS_LED0_PIN;
        #endif
        break;
    case 1:
        #ifdef MRAMS_LED1_PORT
        LAT_CLR(MRAMS_LED1_PORT) = 1<<MRAMS_LED1_PIN;
        #endif
        break;
    case 2:
        #ifdef MRAMS_LED2_PORT
        LAT_CLR(MRAMS_LED2_PORT) = 1<<MRAMS_LED2_PIN;
        #endif
        break;
    case 3:
        #ifdef MRAMS_LED3_PORT
        LAT_CLR(MRAMS_LED3_PORT) = 1<<MRAMS_LED3_PIN;
        #endif
        break;
    }
    return cs;
}

int mrams_write(unsigned int offset, char *data, unsigned bcount)
{
    register unsigned int chip;
    register unsigned int address;
    register unsigned int towrite;
    register unsigned int pass = 0;

    while (bcount > 0) {
        pass++;
        towrite = bcount;
        if (towrite > MRBSIZE)
            towrite = MRBSIZE;

        chip = offset / MRAMS_CHIPSIZE;
        address = (offset<<10) - (chip * (MRAMS_CHIPSIZE*MRBSIZE));

        if (chip >= MRAMS_CHIPS) {
            printf("!!!EIO\n");
            return EIO;
        }

        mr_write_block(chip, address, towrite, data);
        bcount -= towrite;
        offset += (towrite>>MRBLOG2);
        data += towrite;
    }
    return 1;
}

/*
 * Initialize hardware.
 */
static int mrams_init(int spi_port, int cs0, int cs1, int cs2, int cs3)
{
    struct spiio *io = &mrams_io[0];

    if (spi_setup(io, spi_port, cs0) != 0) {
        printf("mr0: cannot open SPI%u port\n", spi_port);
        return 0;
    }
    spi_brg(io, MRAMS_MHZ * 1000);
    spi_set(io, PIC32_SPICON_CKE);
    spi_select(io);
    spi_transfer(io, MRAM_WREN);
    spi_deselect(io);

#if MRAMS_CHIPS >= 1
    spi_setup(io+1, spi_port, cs1);

    spi_brg(io+1, MRAMS_MHZ * 1000);
    spi_set(io+1, PIC32_SPICON_CKE);
    spi_select(io+1);
    spi_transfer(io+1, MRAM_WREN);
    spi_deselect(io+1);
#endif
#if MRAMS_CHIPS >= 2
    spi_setup(io+2, spi_port, cs2);

    spi_brg(io+2, MRAMS_MHZ * 1000);
    spi_set(io+2, PIC32_SPICON_CKE);
    spi_select(io+2);
    spi_transfer(io+2, MRAM_WREN);
    spi_deselect(io+2);
#endif
#if MRAMS_CHIPS >= 3
    spi_setup(io+3, spi_port, cs3);

    spi_brg(io+3, MRAMS_MHZ * 1000);
    spi_set(io+3, PIC32_SPICON_CKE);
    spi_select(io+3);
    spi_transfer(io+3, MRAM_WREN);
    spi_deselect(io+3);
#endif

    printf("mr0: size %dKB, speed %d Mbit/sec\n",
        MRAMS_CHIPS * MRAMS_CHIPSIZE, spi_get_brg(io) / 1000);
    return 1;
}

/*
 * Open the disk.
 */
int mrams_open(dev_t dev, int flag, int mode)
{
    return 0;
}

int mrams_close(dev_t dev, int flag, int mode)
{
    return 0;
}

/*
 * Return the size of the device in kbytes.
 */
daddr_t mrams_size(dev_t dev)
{
    return MRAMS_TOTAL_KBYTES;
}

void mrams_strategy(struct buf *bp)
{
    int offset = bp->b_blkno;
    long nblk = btod(bp->b_bcount);
    int s;

    /*
     * Determine the size of the transfer, and make sure it is
     * within the boundaries of the partition.
     */
    if (bp->b_blkno + nblk > MRAMS_TOTAL_KBYTES) {
        /* if exactly at end of partition, return an EOF */
        if (bp->b_blkno == MRAMS_TOTAL_KBYTES) {
            bp->b_resid = bp->b_bcount;
            biodone(bp);
            return;
        }
        /* or truncate if part of it fits */
        nblk = MRAMS_TOTAL_KBYTES - bp->b_blkno;
        if (nblk <= 0) {
            bp->b_error = EINVAL;
            bp->b_flags |= B_ERROR;
            biodone(bp);
            return;
        }
        bp->b_bcount = nblk << DEV_BSHIFT;
    }

    led_control(LED_SWAP, 1);

    s = splbio();
#ifdef UCB_METER
    if (mrams_dkindex >= 0) {
        dk_busy |= 1 << mrams_dkindex;
        dk_xfer[mrams_dkindex]++;
        dk_bytes[mrams_dkindex] += bp->b_bcount;
    }
#endif

    if (bp->b_flags & B_READ) {
        mrams_read(offset, bp->b_addr, bp->b_bcount);
    } else {
        mrams_write(offset, bp->b_addr, bp->b_bcount);
    }

    biodone(bp);
    led_control(LED_SWAP, 0);
#ifdef UCB_METER
    if (mrams_dkindex >= 0)
        dk_busy &= ~(1 << mrams_dkindex);
#endif
    splx(s);
}

int mrams_ioctl(dev_t dev, u_int cmd, caddr_t addr, int flag)
{
    int error = 0;

    switch (cmd) {

    case DIOCGETMEDIASIZE:
        /* Get disk size in kbytes. */
        *(int*) addr = MRAMS_TOTAL_KBYTES;
        break;

    default:
        error = EINVAL;
        break;
    }
    return error;
}

/*
 * Test to see if device is present.
 * Return true if found and initialized ok.
 */
static int
mrams_probe(config)
    struct conf_device *config;
{
    int cs0 = config->dev_pins[0];
    int cs1 = config->dev_pins[1];
    int cs2 = config->dev_pins[2];
    int cs3 = config->dev_pins[3];

    /* Only one device unit is supported. */
    if (config->dev_unit != 0)
        return 0;

    printf("mr0: port SPI%d, pins cs0=R%c%d/cs1=R%c%d/cs2=R%c%d/cs3=R%c%d\n",
        config->dev_ctlr,
        gpio_portname(cs0), gpio_pinno(cs0),
        gpio_portname(cs1), gpio_pinno(cs1),
        gpio_portname(cs2), gpio_pinno(cs2),
        gpio_portname(cs3), gpio_pinno(cs3));

    if (mrams_init(config->dev_ctlr, cs0, cs1, cs2, cs3) != 0)
        return 0;

#ifdef UCB_METER
    dk_alloc(&mrams_dkindex, 1, "mr0");
#endif
    return 1;
}

struct driver mrdriver = {
    "mr", mrams_probe,
};
