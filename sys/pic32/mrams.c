/*
 * TODO: Modify this driver to be able to function without rdisk layer.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/dk.h>
#include <sys/disk.h>
#include <sys/spi.h>
#include <sys/debug.h>
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

int mrams_size(int unit)
{
    return MRAMS_CHIPS * MRAMS_CHIPSIZE;
}

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

int mrams_read(int unit, unsigned int offset, char *data, unsigned int bcount)
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

int mrams_write(int unit, unsigned int offset, char *data, unsigned bcount)
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

void mrams_preinit(int unit)
{
    struct spiio *io = &mrams_io[0];
    struct buf *bp;

    if (unit >= 1)
        return;

    /* Initialize hardware. */

    if (spi_setup(io, MRAMS_PORT, (unsigned int *)&MRAMS_CS0_PORT, MRAMS_CS0_PIN) != 0)
        return;

    spi_brg(io, MRAMS_MHZ * 1000);
    spi_set(io, PIC32_SPICON_CKE);
    spi_select(io);
    spi_transfer(io, MRAM_WREN);
    spi_deselect(io);

#ifdef MRAMS_CS1_PORT
    spi_setup(io+1, MRAMS_PORT, (unsigned int *)&MRAMS_CS1_PORT, MRAMS_CS1_PIN);

    spi_brg(io+1, MRAMS_MHZ * 1000);
    spi_set(io+1, PIC32_SPICON_CKE);
    spi_select(io+1);
    spi_transfer(io+1, MRAM_WREN);
    spi_deselect(io+1);
#endif
#ifdef MRAMS_CS2_PORT
    spi_setup(io+2, MRAMS_PORT, (unsigned int *)&MRAMS_CS2_PORT, MRAMS_CS2_PIN);

    spi_brg(io+2, MRAMS_MHZ * 1000);
    spi_set(io+2, PIC32_SPICON_CKE);
    spi_select(io+2);
    spi_transfer(io+2, MRAM_WREN);
    spi_deselect(io+2);
#endif
#ifdef MRAMS_CS3_PORT
    spi_setup(io+3, MRAMS_PORT, (unsigned int *)&MRAMS_CS3_PORT, MRAMS_CS3_PIN);

    spi_brg(io+3, MRAMS_MHZ * 1000);
    spi_set(io+3, PIC32_SPICON_CKE);
    spi_select(io+3);
    spi_transfer(io+3, MRAM_WREN);
    spi_deselect(io+3);
#endif

    printf("mrams0: port %s, size %dKB, speed %d Mbit/sec\n",
        spi_name(MRAMS_PORT), MRAMS_CHIPS * MRAMS_CHIPSIZE,
        spi_get_brg(io) / 1000);
    bp = prepartition_device("mrams0");
    if (bp) {
        mrams_write(0, 0, bp->b_addr, 512);
        brelse(bp);
    }
}
