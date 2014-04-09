/*
 * Driver for external RAM-based swap device.
 *
 * Interface:
 *  data[7:0] - connected to PORTx
 *  rd        - fetch a byte from memory to data[7:0], increment address
 *  wr        - write a byte data[7:0] to memory, increment address
 *  ldaddr    - write address from data[7:0] in 3 steps: low-middle-high
 *
 * Signals rd, wr, ldadr are active LOW and idle HIGH.
 * To activate, you need to pulse it high-low-high.
 * CHANGE: IM 23.12.2011 - signals active LOW
 * CHANGE: IM 24.12.2011 - finetuning for 55ns SRAM and 7ns CPLD
 *                       - some nops removed
 *                       - nops marked 55ns are required !!!
 *                       - MAX performance settings for 55ns SRAM
 * CHANGE: IM 28.12.2011 - dev_load_address is 6x4bit now
 */
#include "param.h"
#include "systm.h"
#include "buf.h"
#include "errno.h"
#include "dk.h"
#include "rdisk.h"

#include "debug.h"

int sw_dkn = -1;                /* Statistics slot number */

/*
 * Set data output value.
 */
static inline void data_set (unsigned char byte)
{
        LAT_CLR(SW_DATA_PORT) = 0xff << SW_DATA_PIN;
        LAT_SET(SW_DATA_PORT) = byte << SW_DATA_PIN;
}

/*
 * Switch data bus to input.
 */
static inline void data_switch_input ()
{
        LAT_CLR(SW_DATA_PORT) = 0xff << SW_DATA_PIN;    // !!! PIC32 errata
        TRIS_SET(SW_DATA_PORT) = 0xff << SW_DATA_PIN;
	asm volatile ("nop");
}

/*
 * Switch data bus to output.
 */
static inline void data_switch_output ()
{
        TRIS_CLR(SW_DATA_PORT) = 0xff << SW_DATA_PIN;
	asm volatile ("nop");
}

/*
 * Get data input value.
 */
static inline unsigned char data_get ()
{
	return PORT_VAL(SW_DATA_PORT) >> SW_DATA_PIN;
}

/*
 * Send LDA pulse: high-low-high.
 */
static inline void lda_pulse ()
{
	LAT_CLR(SW_LDA_PORT) = 1 << SW_LDA_PIN;
	asm volatile ("nop");
	asm volatile ("nop");
	LAT_SET(SW_LDA_PORT) = 1 << SW_LDA_PIN;
}

/*
 * Set RD low.
 * Minimal time between falling edge of RD to data valid is 50ns.
 */
static inline void rd_low ()
{
        LAT_CLR(SW_RD_PORT) = 1 << SW_RD_PIN;

#if BUS_KHZ > 33000
        asm volatile ("nop"); // 55ns
 	asm volatile ("nop"); // 55ns
#endif
#if BUS_KHZ > 66000
        asm volatile ("nop"); // 55ns
	asm volatile ("nop"); // 55ns
#endif
}

/*
 * Set RD high.
 */
static inline void rd_high ()
{
        LAT_SET(SW_RD_PORT) = 1 << SW_RD_PIN;
}

/*
 * Send WR pulse: high-low-high.
 * It shall be minimally 40ns.
 */
static inline void wr_pulse ()
{
        LAT_CLR(SW_WR_PORT) = 1 << SW_WR_PIN;

#if BUS_KHZ > 33000
        asm volatile ("nop");  // 55ns
	asm volatile ("nop");  // 55ns
#endif
#if BUS_KHZ > 66000
        asm volatile ("nop");  // 55ns
#endif
        LAT_SET(SW_WR_PORT) = 1 << SW_WR_PIN;
}

/*
 * Load the 24 bit address to ramdisk.
 * Leave data bus in input mode.
 */
static void
dev_load_address (addr)
        unsigned addr;
{
	/* Toggle rd: make one dummy read - this clears cpld's addr pointer */
        rd_low ();
	rd_high ();

	data_switch_output();           /* switch data bus to output */

        data_set (addr);                /* send lowest 4 bits */
        lda_pulse();                    /* pulse ldaddr */

        data_set (addr >> 4);           /* send 4 bits */
        lda_pulse();                    /* pulse ldaddr */

        data_set (addr >> 8);           /* send 4 bits */
        lda_pulse();                    /* pulse ldaddr */

	data_set (addr >> 12);          /* send 4 bits */
        lda_pulse();                    /* pulse ldaddr */

        data_set (addr >> 16);          /* send 4 bits */
        lda_pulse();                    /* pulse ldaddr */

        data_set (addr >> 20);          /* send highest 4 bits */
        lda_pulse();                    /* pulse ldaddr */

	data_switch_input();
}

/*
 * Get number of kBytes on the disk.
 * Return nonzero if successful.
 */
int sramc_size ( int unit )
{
	return 4096;
}

/*
 * Read a block of data.
 */
int sramc_read (int unit, unsigned int blockno, char *data, unsigned int nbytes)
{
	int i;

	//DEBUG9("sramc%d: read block %u, length %u bytes, addr %p\n",
	//	major(dev), blockno, nbytes, data);

	dev_load_address (blockno * DEV_BSIZE);

        data_switch_input();            /* switch data bus to input */

	/* Read data. */
        for (i=0; i<nbytes; i++) {
                rd_low();               /* set rd LOW */

                *data++ = data_get();   /* read a byte of data */

                rd_high();              /* set rd HIGH */
        }
	return 1;
}

/*
 * Write a block of data.
 */
int sramc_write (int unit, unsigned int blockno, char *data, unsigned int nbytes)
{
	unsigned i;

	//DEBUG9("sramc%d: write block %u, length %u bytes, addr %p\n",
	//	major(dev), blockno, nbytes, data);

	dev_load_address (blockno * DEV_BSIZE);

	data_switch_output();           /* switch data bus to output */

        for (i=0; i<nbytes; i++) {
                data_set (*data);       /* send a byte of data */
                data++;

                wr_pulse();             /* pulse wr */
        }

        /* Switch data bus to input. */
        data_switch_input();
	return 1;
}


/*
 * Init the disk.
 */
void sramc_init (int unit)
{
	struct buf *bp;
	if (TRIS_VAL(SW_LDA_PORT) & (1 << SW_LDA_PIN)) {
		/* Initialize hardware.
                 * Switch data bus to input. */
                data_switch_input();

                /* Set idle HIGH rd, wr and ldaddr as output pins. */
		LAT_SET(SW_RD_PORT) = 1 << SW_RD_PIN;
		LAT_SET(SW_WR_PORT) = 1 << SW_WR_PIN;
		LAT_SET(SW_LDA_PORT) = 1 << SW_LDA_PIN;
		TRIS_CLR(SW_RD_PORT) = 1 << SW_RD_PIN;
		TRIS_CLR(SW_WR_PORT) = 1 << SW_WR_PIN;
		TRIS_CLR(SW_LDA_PORT) = 1 << SW_LDA_PIN;

                /* Toggle rd: make one dummy read. */
                rd_low();              /* set rd low */
                rd_high();             /* set rd high */
        }
	DEBUG3("sramc%d: init done\n",unit);
        bp = prepartition_device("sramc0");
        if(bp)
        {
                sramc_write(0, 0, bp->b_addr, 512);
                brelse(bp);
        }

	return;
}

/*
 * Open the disk.
 */
int sramc_open (int unit)
{
	DEBUG3("sramc%d: open\n",unit);
	return 0;
}
