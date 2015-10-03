/*
 * Disk driver for serial RAM chips connected via SPI port.
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
#include <machine/spirams.h>

#define SPIRAM_WREN     0x06
#define SPIRAM_WRDI     0x04
#define SPIRAM_RDSR     0x05
#define SPIRAM_WRSR     0x01
#define SPIRAM_READ     0x03
#define SPIRAM_WRITE    0x02
#define SPIRAM_SLEEP    0xB9
#define SPIRAM_WAKE     0xAB

#ifndef SPIRAMS_MHZ
#define SPIRAMS_MHZ     10
#endif

struct spiio spirams_io[SPIRAMS_CHIPS];

int spirams_dkindex;                    /* disk index for statistics */

/*
 * Size of RAM disk.
 */
#define SPIRAMS_TOTAL_KBYTES    (SPIRAMS_CHIPS * SPIRAMS_CHIPSIZE)

#define MRBSIZE         1024
#define MRBLOG2         10

unsigned int spir_read_block(unsigned int chip, unsigned int address, unsigned int length, char *data)
{
    struct spiio *io = &spirams_io[chip];
    register unsigned int cs = 0;

    switch (chip) {
    case 0:
        #ifdef SPIRAMS_LED0_PORT
        TRIS_CLR(SPIRAMS_LED0_PORT) = 1<<SPIRAMS_LED0_PIN;
        LAT_SET(SPIRAMS_LED0_PORT) = 1<<SPIRAMS_LED0_PIN;
        #endif
        break;
    case 1:
        #ifdef SPIRAMS_LED1_PORT
        TRIS_CLR(SPIRAMS_LED1_PORT) = 1<<SPIRAMS_LED1_PIN;
        LAT_SET(SPIRAMS_LED1_PORT) = 1<<SPIRAMS_LED1_PIN;
        #endif
        break;
    case 2:
        #ifdef SPIRAMS_LED2_PORT
        TRIS_CLR(SPIRAMS_LED2_PORT) = 1<<SPIRAMS_LED2_PIN;
        LAT_SET(SPIRAMS_LED2_PORT) = 1<<SPIRAMS_LED2_PIN;
        #endif
        break;
    case 3:
        #ifdef SPIRAMS_LED3_PORT
        TRIS_CLR(SPIRAMS_LED3_PORT) = 1<<SPIRAMS_LED3_PIN;
        LAT_SET(SPIRAMS_LED3_PORT) = 1<<SPIRAMS_LED3_PIN;
        #endif
        break;
    case 4:
        #ifdef SPIRAMS_LED4_PORT
        TRIS_CLR(SPIRAMS_LED4_PORT) = 1<<SPIRAMS_LED4_PIN;
        LAT_SET(SPIRAMS_LED4_PORT) = 1<<SPIRAMS_LED4_PIN;
        #endif
        break;
    case 5:
        #ifdef SPIRAMS_LED5_PORT
        TRIS_CLR(SPIRAMS_LED5_PORT) = 1<<SPIRAMS_LED5_PIN;
        LAT_SET(SPIRAMS_LED5_PORT) = 1<<SPIRAMS_LED5_PIN;
        #endif
        break;
    case 6:
        #ifdef SPIRAMS_LED6_PORT
        TRIS_CLR(SPIRAMS_LED6_PORT) = 1<<SPIRAMS_LED6_PIN;
        LAT_SET(SPIRAMS_LED6_PORT) = 1<<SPIRAMS_LED6_PIN;
        #endif
        break;
    case 7:
        #ifdef SPIRAMS_LED7_PORT
        TRIS_CLR(SPIRAMS_LED7_PORT) = 1<<SPIRAMS_LED7_PIN;
        LAT_SET(SPIRAMS_LED7_PORT) = 1<<SPIRAMS_LED7_PIN;
        #endif
        break;
    case 8:
        #ifdef SPIRAMS_LED8_PORT
        TRIS_CLR(SPIRAMS_LED8_PORT) = 1<<SPIRAMS_LED8_PIN;
        LAT_SET(SPIRAMS_LED8_PORT) = 1<<SPIRAMS_LED8_PIN;
        #endif
        break;
    case 9:
        #ifdef SPIRAMS_LED9_PORT
        TRIS_CLR(SPIRAMS_LED9_PORT) = 1<<SPIRAMS_LED9_PIN;
        LAT_SET(SPIRAMS_LED9_PORT) = 1<<SPIRAMS_LED9_PIN;
        #endif
        break;
    case 10:
        #ifdef SPIRAMS_LED10_PORT
        TRIS_CLR(SPIRAMS_LED10_PORT) = 1<<SPIRAMS_LED10_PIN;
        LAT_SET(SPIRAMS_LED10_PORT) = 1<<SPIRAMS_LED10_PIN;
        #endif
        break;
    case 11:
        #ifdef SPIRAMS_LED11_PORT
        TRIS_CLR(SPIRAMS_LED11_PORT) = 1<<SPIRAMS_LED11_PIN;
        LAT_SET(SPIRAMS_LED11_PORT) = 1<<SPIRAMS_LED11_PIN;
        #endif
        break;
    case 12:
        #ifdef SPIRAMS_LED12_PORT
        TRIS_CLR(SPIRAMS_LED12_PORT) = 1<<SPIRAMS_LED12_PIN;
        LAT_SET(SPIRAMS_LED12_PORT) = 1<<SPIRAMS_LED12_PIN;
        #endif
        break;
    case 13:
        #ifdef SPIRAMS_LED13_PORT
        TRIS_CLR(SPIRAMS_LED13_PORT) = 1<<SPIRAMS_LED13_PIN;
        LAT_SET(SPIRAMS_LED13_PORT) = 1<<SPIRAMS_LED13_PIN;
        #endif
        break;
    case 14:
        #ifdef SPIRAMS_LED14_PORT
        TRIS_CLR(SPIRAMS_LED14_PORT) = 1<<SPIRAMS_LED14_PIN;
        LAT_SET(SPIRAMS_LED14_PORT) = 1<<SPIRAMS_LED14_PIN;
        #endif
        break;
    case 15:
        #ifdef SPIRAMS_LED15_PORT
        TRIS_CLR(SPIRAMS_LED15_PORT) = 1<<SPIRAMS_LED15_PIN;
        LAT_SET(SPIRAMS_LED15_PORT) = 1<<SPIRAMS_LED15_PIN;
        #endif
        break;
    }

    spi_select(io);
    spi_transfer(io, SPIRAM_READ);
    spi_transfer(io, address >> 16);
    spi_transfer(io, address >> 8);
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
        #ifdef SPIRAMS_LED0_PORT
        LAT_CLR(SPIRAMS_LED0_PORT) = 1<<SPIRAMS_LED0_PIN;
        #endif
        break;
    case 1:
        #ifdef SPIRAMS_LED1_PORT
        LAT_CLR(SPIRAMS_LED1_PORT) = 1<<SPIRAMS_LED1_PIN;
        #endif
        break;
    case 2:
        #ifdef SPIRAMS_LED2_PORT
        LAT_CLR(SPIRAMS_LED2_PORT) = 1<<SPIRAMS_LED2_PIN;
        #endif
        break;
    case 3:
        #ifdef SPIRAMS_LED3_PORT
        LAT_CLR(SPIRAMS_LED3_PORT) = 1<<SPIRAMS_LED3_PIN;
        #endif
        break;
    }
    return cs;
}

int spirams_read(unsigned int offset, char *data, unsigned int bcount)
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

        chip = offset / SPIRAMS_CHIPSIZE;

        address = (offset<<10) - (chip * (SPIRAMS_CHIPSIZE*1024));

        if (chip >= SPIRAMS_CHIPS) {
            printf("!!!EIO\n");
            return EIO;
        }
        spir_read_block(chip, address, toread, data);
        bcount -= toread;
        offset += (toread>>MRBLOG2);
        data += toread;
    }
    return 1;
}

unsigned int spir_write_block(unsigned int chip, unsigned int address, unsigned int length, char *data)
{
    struct spiio *io = &spirams_io[chip];
    register unsigned int cs = 0;
    char blank __attribute__((unused));

    switch (chip) {
    case 0:
        #ifdef SPIRAMS_LED0_PORT
        TRIS_CLR(SPIRAMS_LED0_PORT) = 1<<SPIRAMS_LED0_PIN;
        LAT_SET(SPIRAMS_LED0_PORT) = 1<<SPIRAMS_LED0_PIN;
        #endif
        break;
    case 1:
        #ifdef SPIRAMS_LED1_PORT
        TRIS_CLR(SPIRAMS_LED1_PORT) = 1<<SPIRAMS_LED1_PIN;
        LAT_SET(SPIRAMS_LED1_PORT) = 1<<SPIRAMS_LED1_PIN;
        #endif
        break;
    case 2:
        #ifdef SPIRAMS_LED2_PORT
        TRIS_CLR(SPIRAMS_LED2_PORT) = 1<<SPIRAMS_LED2_PIN;
        LAT_SET(SPIRAMS_LED2_PORT) = 1<<SPIRAMS_LED2_PIN;
        #endif
        break;
    case 3:
        #ifdef SPIRAMS_LED3_PORT
        TRIS_CLR(SPIRAMS_LED3_PORT) = 1<<SPIRAMS_LED3_PIN;
        LAT_SET(SPIRAMS_LED3_PORT) = 1<<SPIRAMS_LED3_PIN;
        #endif
        break;
    }

    spi_select(io);
    spi_transfer(io, SPIRAM_WRITE);
    spi_transfer(io, address >> 16);
    spi_transfer(io, address >> 8);
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
        #ifdef SPIRAMS_LED0_PORT
        LAT_CLR(SPIRAMS_LED0_PORT) = 1<<SPIRAMS_LED0_PIN;
        #endif
        break;
    case 1:
        #ifdef SPIRAMS_LED1_PORT
        LAT_CLR(SPIRAMS_LED1_PORT) = 1<<SPIRAMS_LED1_PIN;
        #endif
        break;
    case 2:
        #ifdef SPIRAMS_LED2_PORT
        LAT_CLR(SPIRAMS_LED2_PORT) = 1<<SPIRAMS_LED2_PIN;
        #endif
        break;
    case 3:
        #ifdef SPIRAMS_LED3_PORT
        LAT_CLR(SPIRAMS_LED3_PORT) = 1<<SPIRAMS_LED3_PIN;
        #endif
        break;
    }
    return cs;
}

int spirams_write (unsigned int offset, char *data, unsigned bcount)
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

        chip = offset / SPIRAMS_CHIPSIZE;
        address = (offset<<10) - (chip * (SPIRAMS_CHIPSIZE*MRBSIZE));

        if (chip >= SPIRAMS_CHIPS) {
            printf("!!!EIO\n");
            return EIO;
        }

        spir_write_block(chip, address, towrite, data);
        bcount -= towrite;
        offset += (towrite>>MRBLOG2);
        data += towrite;
    }
    return 1;
}

/*
 * Initialize hardware.
 */
static int spirams_init(int spi_port, char cs[])
{
    struct spiio *io = &spirams_io[0];

    if (spi_setup(io, spi_port, cs[0]) != 0) {
        printf("sr0: cannot open SPI%u port\n", spi_port);
        return 0;
    }
    spi_brg(io, SPIRAMS_MHZ * 1000);
    spi_set(io, PIC32_SPICON_CKE);

#if SPIRAMS_CHIPS >= 1
    spi_setup(io+1, spi_port, cs[1]);

    spi_brg(io+1, SPIRAMS_MHZ * 1000);
    spi_set(io+1, PIC32_SPICON_CKE);
#endif
#if SPIRAMS_CHIPS >= 2
    spi_setup(io+2, spi_port, cs[2]);

    spi_brg(io+2, SPIRAMS_MHZ * 1000);
    spi_set(io+2, PIC32_SPICON_CKE);
#endif
#if SPIRAMS_CHIPS >= 3
    spi_setup(io+3, spi_port, cs[3]);

    spi_brg(io+3, SPIRAMS_MHZ * 1000);
    spi_set(io+3, PIC32_SPICON_CKE);
#endif
#if SPIRAMS_CHIPS >= 4
    spi_setup(io+4, spi_port, cs[4]);

    spi_brg(io+4, SPIRAMS_MHZ * 1000);
    spi_set(io+4, PIC32_SPICON_CKE);
#endif
#if SPIRAMS_CHIPS >= 5
    spi_setup(io+5, spi_port, cs[5]);

    spi_brg(io+5, SPIRAMS_MHZ * 1000);
    spi_set(io+5, PIC32_SPICON_CKE);
#endif
#if SPIRAMS_CHIPS >= 6
    spi_setup(io+6, spi_port, cs[6]);

    spi_brg(io+6, SPIRAMS_MHZ * 1000);
    spi_set(io+6, PIC32_SPICON_CKE);
#endif
#if SPIRAMS_CHIPS >= 7
    spi_setup(io+7, spi_port, cs[7]);

    spi_brg(io+7, SPIRAMS_MHZ * 1000);
    spi_set(io+7, PIC32_SPICON_CKE);
#endif
#if SPIRAMS_CHIPS >= 8
    spi_setup(io+8, spi_port, cs[8]);

    spi_brg(io+8, SPIRAMS_MHZ * 1000);
    spi_set(io+8, PIC32_SPICON_CKE);
#endif
#if SPIRAMS_CHIPS >= 9
    spi_setup(io+9, spi_port, cs[9]);

    spi_brg(io+9, SPIRAMS_MHZ * 1000);
    spi_set(io+9, PIC32_SPICON_CKE);
#endif
#if SPIRAMS_CHIPS >= 10
    spi_setup(io+10, spi_port, cs[10]);

    spi_brg(io+10, SPIRAMS_MHZ * 1000);
    spi_set(io+10, PIC32_SPICON_CKE);
#endif
#if SPIRAMS_CHIPS >= 11
    spi_setup(io+11, spi_port, cs[11]);

    spi_brg(io+11, SPIRAMS_MHZ * 1000);
    spi_set(io+11, PIC32_SPICON_CKE);
#endif
#if SPIRAMS_CHIPS >= 12
    spi_setup(io+12, spi_port, cs[12]);

    spi_brg(io+12, SPIRAMS_MHZ * 1000);
    spi_set(io+12, PIC32_SPICON_CKE);
#endif
#if SPIRAMS_CHIPS >= 13
    spi_setup(io+13, spi_port, cs[13]);

    spi_brg(io+13, SPIRAMS_MHZ * 1000);
    spi_set(io+13, PIC32_SPICON_CKE);
#endif
#if SPIRAMS_CHIPS >= 14
    spi_setup(io+14, spi_port, cs[14]);

    spi_brg(io+14, SPIRAMS_MHZ * 1000);
    spi_set(io+14, PIC32_SPICON_CKE);
#endif
#if SPIRAMS_CHIPS >= 15
    spi_setup(io+15, spi_port, cs[15]);

    spi_brg(io+15, SPIRAMS_MHZ * 1000);
    spi_set(io+15, PIC32_SPICON_CKE);
#endif

    printf("spirams0: size %dKB, speed %d Mbit/sec\n",
        SPIRAMS_CHIPS * SPIRAMS_CHIPSIZE, spi_get_brg(io) / 1000);
    return 1;
}

/*
 * Open the disk.
 */
int spirams_open(dev_t dev, int flag, int mode)
{
    return 0;
}

int spirams_close(dev_t dev, int flag, int mode)
{
    return 0;
}

/*
 * Return the size of the device in kbytes.
 */
daddr_t spirams_size(dev_t dev)
{
    return SPIRAMS_TOTAL_KBYTES;
}

void spirams_strategy(struct buf *bp)
{
    int offset = bp->b_blkno;
    long nblk = btod(bp->b_bcount);
    int s;

    /*
     * Determine the size of the transfer, and make sure it is
     * within the boundaries of the partition.
     */
    if (bp->b_blkno + nblk > SPIRAMS_TOTAL_KBYTES) {
        /* if exactly at end of partition, return an EOF */
        if (bp->b_blkno == SPIRAMS_TOTAL_KBYTES) {
            bp->b_resid = bp->b_bcount;
            biodone(bp);
            return;
        }
        /* or truncate if part of it fits */
        nblk = SPIRAMS_TOTAL_KBYTES - bp->b_blkno;
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
    if (spirams_dkindex >= 0) {
        dk_busy |= 1 << spirams_dkindex;
        dk_xfer[spirams_dkindex]++;
        dk_bytes[spirams_dkindex] += bp->b_bcount;
    }
#endif

    if (bp->b_flags & B_READ) {
        spirams_read(offset, bp->b_addr, bp->b_bcount);
    } else {
        spirams_write(offset, bp->b_addr, bp->b_bcount);
    }

    biodone(bp);
    led_control(LED_SWAP, 0);
#ifdef UCB_METER
    if (spirams_dkindex >= 0)
        dk_busy &= ~(1 << spirams_dkindex);
#endif
    splx(s);
}

int spirams_ioctl(dev_t dev, u_int cmd, caddr_t addr, int flag)
{
    int error = 0;

    switch (cmd) {

    case DIOCGETMEDIASIZE:
        /* Get disk size in kbytes. */
        *(int*) addr = SPIRAMS_TOTAL_KBYTES;
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
spirams_probe(config)
    struct conf_device *config;
{
    int i;

    /* Only one device unit is supported. */
    if (config->dev_unit != 0)
        return 0;

    printf("sr0: port SPI%d, pins ", config->dev_ctlr);
    for (i=0; i<SPIRAMS_CHIPS; i++) {
        int cs = config->dev_pins[i];
        if (i > 0)
            printf("/");
        if (i == 7)
            printf("\n                     ");
        printf("R%c%d", gpio_portname(cs), gpio_pinno(cs));
    }
    printf("\n");
    if (spirams_init(config->dev_ctlr, config->dev_pins) != 0)
        return 0;

#ifdef UCB_METER
    dk_alloc(&spirams_dkindex, 1, "sr0");
#endif
    return 1;
}

struct driver srdriver = {
    "sr", spirams_probe,
};
