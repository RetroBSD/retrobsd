#include "param.h"
#include "systm.h"
#include "buf.h"
#include "errno.h"
#include "dk.h"
#include "rdisk.h"
#include "spi_bus.h"

#include "debug.h"

#define MRAM_WREN	0x06
#define MRAM_WRDI	0x04
#define MRAM_RDSR	0x05
#define MRAM_WRSR	0x01
#define MRAM_READ	0x03
#define MRAM_WRITE	0x02
#define MRAM_SLEEP	0xB9
#define MRAM_WAKE	0xAB

#ifndef MRAMS_MHZ
#define MRAMS_MHZ 13
#endif

int fd[MRAMS_CHIPS];

int mrams_size(int unit)
{
	return MRAMS_CHIPS * MRAMS_CHIPSIZE;
}

#define MRBSIZE 1024
#define MRBLOG2 10

unsigned int mr_read_block(unsigned int chip, unsigned int address, unsigned int length, char *data)
{
    register unsigned int cs = 0;

    switch(chip)
    {
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

    spi_select(fd[chip]);
    spi_transfer(fd[chip], MRAM_READ);
    spi_transfer(fd[chip], address>>16);
    spi_transfer(fd[chip], address>>8);
    spi_transfer(fd[chip], address);

    // If the length is a multiple of 32 bits, then do a 32 bit transfer
/*
    if((length & 0x03) == 0)
        spi_bulk_read_32(fd[chip],length,data);
    else if((length & 0x01) == 0)
        spi_bulk_read_16(fd[chip],length,data);
    else 
*/
        spi_bulk_read(fd[chip],length,(unsigned char *)data);

	spi_deselect(fd[chip]);

    switch(chip)
    {
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

    while(bcount > 0)
    {
        pass++;
        toread = bcount;
        if(toread > MRBSIZE)
            toread = MRBSIZE;

        chip = offset / MRAMS_CHIPSIZE;

        address = (offset<<10) - (chip * (MRAMS_CHIPSIZE*1024));


        if(chip>=MRAMS_CHIPS)
        {
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
    register unsigned int cs = 0;
    char blank __attribute__((unused));

    switch(chip)
    {
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

    spi_select(fd[chip]);
    spi_transfer(fd[chip], MRAM_WRITE);
    spi_transfer(fd[chip], address>>16);
    spi_transfer(fd[chip], address>>8);
    spi_transfer(fd[chip], address);

/*
    if((length & 0x03) == 0)
        spi_bulk_write_32(fd[chip],length,data);
    else if((length & 0x01) == 0)
        spi_bulk_write_16(fd[chip],length,data);
    else 
*/
        spi_bulk_write(fd[chip],length,(unsigned char *)data);

    spi_deselect(fd[chip]);

    switch(chip)
    {
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

int mrams_write (int unit, unsigned int offset, char *data, unsigned bcount)
{
    register unsigned int chip;
    register unsigned int address;
    register unsigned int towrite;
    register unsigned int pass = 0;

    while(bcount > 0)
    {
        pass++;
        towrite = bcount;
        if(towrite > MRBSIZE)
            towrite = MRBSIZE;

        chip = offset / MRAMS_CHIPSIZE;
        address = (offset<<10) - (chip * (MRAMS_CHIPSIZE*MRBSIZE));


        if(chip>=MRAMS_CHIPS)
        {
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

void mrams_preinit (int unit)
{
	struct buf *bp;

	if (unit >= 1)
		return;

	/* Initialize hardware. */

    
    fd[0] = spi_open(MRAMS_PORT,(unsigned int *)&MRAMS_CS0_PORT,MRAMS_CS0_PIN);
    if(fd[0]==-1) return;

    spi_brg(fd[0],MRAMS_MHZ * 1000);
    spi_set(fd[0],PIC32_SPICON_CKE);
    spi_select(fd[0]);
    spi_transfer(fd[0],MRAM_WREN);
    spi_deselect(fd[0]);

#ifdef MRAMS_CS1_PORT
    fd[1] = spi_open(MRAMS_PORT,(unsigned int *)&MRAMS_CS1_PORT,MRAMS_CS1_PIN);

    spi_brg(fd[1],MRAMS_MHZ * 1000);
    spi_set(fd[1],PIC32_SPICON_CKE);
    spi_select(fd[1]);
    spi_transfer(fd[1],MRAM_WREN);
    spi_deselect(fd[1]);
#endif
#ifdef MRAMS_CS2_PORT
    fd[2] = spi_open(MRAMS_PORT,(unsigned int *)&MRAMS_CS2_PORT,MRAMS_CS2_PIN);

    spi_brg(fd[2],MRAMS_MHZ * 1000);
    spi_set(fd[2],PIC32_SPICON_CKE);
    spi_select(fd[2]);
    spi_transfer(fd[2],MRAM_WREN);
    spi_deselect(fd[2]);
#endif
#ifdef MRAMS_CS3_PORT
    fd[3] = spi_open(MRAMS_PORT,(unsigned int *)&MRAMS_CS3_PORT,MRAMS_CS3_PIN);

    spi_brg(fd[3],MRAMS_MHZ * 1000);
    spi_set(fd[3],PIC32_SPICON_CKE);
    spi_select(fd[3]);
    spi_transfer(fd[3],MRAM_WREN);
    spi_deselect(fd[3]);
#endif

	printf("mrams0: port %s, size %dKB, speed %d Mbit/sec\n",
		spi_name(MRAMS_PORT),MRAMS_CHIPS * MRAMS_CHIPSIZE,
		spi_get_brg(fd[0]) / 1000);
	bp = prepartition_device("mrams0");
	if(bp)
	{
		mrams_write (0, 0, bp->b_addr, 512);
		brelse(bp);
	}
}
