#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/dk.h>
#include <sys/rdisk.h>
#include <sys/spi.h>
#include <sys/debug.h>

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

int spirams_size(int unit)
{
    return SPIRAMS_CHIPS * SPIRAMS_CHIPSIZE;
}

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

int spirams_read(int unit, unsigned int offset, char *data, unsigned int bcount)
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

int spirams_write (int unit, unsigned int offset, char *data, unsigned bcount)
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

void spirams_preinit (int unit)
{
    struct spiio *io = &spirams_io[0];
    struct buf *bp;

    if (unit >= 1)
        return;

    /* Initialize hardware. */

    if (spi_setup(io, SPIRAMS_PORT,(unsigned int *)&SPIRAMS_CS0_PORT,SPIRAMS_CS0_PIN) != 0)
        return;

    spi_brg(io, SPIRAMS_MHZ * 1000);
    spi_set(io, PIC32_SPICON_CKE);

#ifdef SPIRAMS_CS1_PORT
    spi_setup(io+1, SPIRAMS_PORT,(unsigned int *)&SPIRAMS_CS1_PORT,SPIRAMS_CS1_PIN);

    spi_brg(io+1, SPIRAMS_MHZ * 1000);
    spi_set(io+1, PIC32_SPICON_CKE);
#endif
#ifdef SPIRAMS_CS2_PORT
    spi_setup(io+2, SPIRAMS_PORT,(unsigned int *)&SPIRAMS_CS2_PORT,SPIRAMS_CS2_PIN);

    spi_brg(io+2, SPIRAMS_MHZ * 1000);
    spi_set(io+2, PIC32_SPICON_CKE);
#endif
#ifdef SPIRAMS_CS3_PORT
    spi_setup(io+3, SPIRAMS_PORT,(unsigned int *)&SPIRAMS_CS3_PORT,SPIRAMS_CS3_PIN);

    spi_brg(io+3, SPIRAMS_MHZ * 1000);
    spi_set(io+3, PIC32_SPICON_CKE);
#endif
#ifdef SPIRAMS_CS4_PORT
    spi_setup(io+4, SPIRAMS_PORT,(unsigned int *)&SPIRAMS_CS4_PORT,SPIRAMS_CS4_PIN);

    spi_brg(io+4, SPIRAMS_MHZ * 1000);
    spi_set(io+4, PIC32_SPICON_CKE);
#endif
#ifdef SPIRAMS_CS5_PORT
    spi_setup(io+5, SPIRAMS_PORT,(unsigned int *)&SPIRAMS_CS5_PORT,SPIRAMS_CS5_PIN);

    spi_brg(io+5, SPIRAMS_MHZ * 1000);
    spi_set(io+5, PIC32_SPICON_CKE);
#endif
#ifdef SPIRAMS_CS6_PORT
    spi_setup(io+6, SPIRAMS_PORT,(unsigned int *)&SPIRAMS_CS6_PORT,SPIRAMS_CS6_PIN);

    spi_brg(io+6, SPIRAMS_MHZ * 1000);
    spi_set(io+6, PIC32_SPICON_CKE);
#endif
#ifdef SPIRAMS_CS7_PORT
    spi_setup(io+7, SPIRAMS_PORT,(unsigned int *)&SPIRAMS_CS7_PORT,SPIRAMS_CS7_PIN);

    spi_brg(io+7, SPIRAMS_MHZ * 1000);
    spi_set(io+7, PIC32_SPICON_CKE);
#endif
#ifdef SPIRAMS_CS8_PORT
    spi_setup(io+8, SPIRAMS_PORT,(unsigned int *)&SPIRAMS_CS8_PORT,SPIRAMS_CS8_PIN);

    spi_brg(io+8, SPIRAMS_MHZ * 1000);
    spi_set(io+8, PIC32_SPICON_CKE);
#endif
#ifdef SPIRAMS_CS9_PORT
    spi_setup(io+9, SPIRAMS_PORT,(unsigned int *)&SPIRAMS_CS9_PORT,SPIRAMS_CS9_PIN);

    spi_brg(io+9, SPIRAMS_MHZ * 1000);
    spi_set(io+9, PIC32_SPICON_CKE);
#endif
#ifdef SPIRAMS_CS10_PORT
    spi_setup(io+10, SPIRAMS_PORT,(unsigned int *)&SPIRAMS_CS10_PORT,SPIRAMS_CS10_PIN);

    spi_brg(io+10, SPIRAMS_MHZ * 1000);
    spi_set(io+10, PIC32_SPICON_CKE);
#endif
#ifdef SPIRAMS_CS11_PORT
    spi_setup(io+11, SPIRAMS_PORT,(unsigned int *)&SPIRAMS_CS11_PORT,SPIRAMS_CS11_PIN);

    spi_brg(io+11, SPIRAMS_MHZ * 1000);
    spi_set(io+11, PIC32_SPICON_CKE);
#endif
#ifdef SPIRAMS_CS12_PORT
    spi_setup(io+12, SPIRAMS_PORT,(unsigned int *)&SPIRAMS_CS12_PORT,SPIRAMS_CS12_PIN);

    spi_brg(io+12, SPIRAMS_MHZ * 1000);
    spi_set(io+12, PIC32_SPICON_CKE);
#endif
#ifdef SPIRAMS_CS13_PORT
    spi_setup(io+13, SPIRAMS_PORT,(unsigned int *)&SPIRAMS_CS13_PORT,SPIRAMS_CS13_PIN);

    spi_brg(io+13, SPIRAMS_MHZ * 1000);
    spi_set(io+13, PIC32_SPICON_CKE);
#endif
#ifdef SPIRAMS_CS14_PORT
    spi_setup(io+14, SPIRAMS_PORT,(unsigned int *)&SPIRAMS_CS14_PORT,SPIRAMS_CS14_PIN);

    spi_brg(io+14, SPIRAMS_MHZ * 1000);
    spi_set(io+14, PIC32_SPICON_CKE);
#endif
#ifdef SPIRAMS_CS15_PORT
    spi_setup(io+15, SPIRAMS_PORT,(unsigned int *)&SPIRAMS_CS15_PORT,SPIRAMS_CS15_PIN);

    spi_brg(io+15, SPIRAMS_MHZ * 1000);
    spi_set(io+15, PIC32_SPICON_CKE);
#endif

    printf("spirams0: port %d %s, size %dKB, speed %d Mbit/sec\n",
        SPIRAMS_PORT, spi_name(io), SPIRAMS_CHIPS * SPIRAMS_CHIPSIZE,
        spi_get_brg(io) / 1000);
    bp = prepartition_device("spirams0");
    if (bp) {
        spirams_write (0, 0, bp->b_addr, 512);
        brelse(bp);
    }
}
