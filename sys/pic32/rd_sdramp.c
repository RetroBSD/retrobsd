/*
 * Driver for external SDRAM-based swap device.
 *
 * See sdram.S for information on interface to sdram
 *
 * This code could use a bit of optimization.
 */

#include "param.h"
#include "systm.h"
#include "buf.h"
#include "errno.h"
#include "dk.h"
#include "sdram.h"
#include "rd_sdramp.h"
#include "rdisk.h"

/*
 * See rd_sdramp_config.h for sdramp port/pin configuration
 */
#include "rd_sdramp_config.h"

int sw_dkn = -1;                /* Statistics slot number */

/*
 * physical specs of SDRAM chip
 */
#define RAM_COLS 512

#if SDR_ADDRESS_LINES == 13
#define RAM_ROWS (4096*2)
#elif SDR_ADDRESS_LINES == 12
#define RAM_ROWS 4096
#elif SDR_ADDRESS_LINES == 11
#define RAM_ROWS 2048
#else
#error Invalid Configuration - SDR_ADDRESS_LINES
#endif

#define RAM_BANKS 4


/*
 * RAM_BURST_COUNT MUST be match the number of bytes
 * read/written by each call to sdram_read/sdram_write
 */ 
#define RAM_BURST_COUNT 8

/*
 * CHUNK_SIZE number of bytes in each "chunk"
 */
#define CHUNK_SIZE 32

#define RAM_BURST_GROUP_COUNT 4  

#define BLOCKS_PER_ROW ( RAM_COLS / CHUNK_SIZE )
  
static char swaptemp[CHUNK_SIZE];

/*
 * Used to specify partition table for ramdisk from Makefile
 */
#define RAMDISK_PARTSPEC(n,t,s,l) 	\
m->partitions[n].type=t; 		\
m->partitions[n].lbastart=s;		\
m->partitions[n].lbalength=l;

//void build_ramdisk_mbr( struct mbr* m );

// FIXME - FOLLOWING shared with gpio.c - needs to be made common

/*
 * PIC32 port i/o registers.
 */
struct gpioreg {
        volatile unsigned tris;		/* Mask of inputs */
        volatile unsigned trisclr;
        volatile unsigned trisset;
        volatile unsigned trisinv;
        volatile unsigned port;		/* Read inputs, write outputs */
        volatile unsigned portclr;
        volatile unsigned portset;
        volatile unsigned portinv;
        volatile unsigned lat;		/* Read/write outputs */
        volatile unsigned latclr;
        volatile unsigned latset;
        volatile unsigned latinv;
        volatile unsigned odc;		/* Open drain configuration */
        volatile unsigned odcclr;
        volatile unsigned odcset;
        volatile unsigned odcinv;
};

struct ocreg {
        volatile unsigned con;		/* ? */
        volatile unsigned conclr;
        volatile unsigned conset;
        volatile unsigned coninv;
        volatile unsigned r;		/* ? */
        volatile unsigned rclr;
        volatile unsigned rset;
        volatile unsigned rinv;
        volatile unsigned rs;		/* ? */
        volatile unsigned rsclr;
        volatile unsigned rsset;
        volatile unsigned rsinv;
};

static void sdram_bank_c(unsigned bank)
{
	// In order to keep unnecessary noise from occuring on the
	// address lines, don't use the hardware set/clear functions.
	// Rather, read the latch value, change it, and write it back.

	struct gpioreg * bankport = (struct gpioreg *)&SDR_BANK_PORT;
	unsigned v = bankport->lat;
	v &= ~(BANK_BITMASK << SDR_BANK_0_BIT);
	v |= (bank & BANK_BITMASK) << SDR_BANK_0_BIT;
	bankport->lat = v;
}

static void sdram_upperlowerbyte( unsigned bit )
{
	struct gpioreg * dqm_port = (struct gpioreg *)&SDR_DQM_PORT;
#ifdef SDR_DQM_UDQM_BIT

	if( bit == 0 )
	{
		dqm_port->latset = (1<<SDR_DQM_UDQM_BIT);
		dqm_port->latclr = (1<<SDR_DQM_LDQM_BIT);
	}
	else
	{
		dqm_port->latset = (1<<SDR_DQM_LDQM_BIT);
		dqm_port->latclr = (1<<SDR_DQM_UDQM_BIT);
	}

//		dqm_port->latset = (1<<SDR_DQM_UDQM_BIT);
//		dqm_port->latclr = (1<<SDR_DQM_LDQM_BIT);
#else
	dqm_port->latclr = (1<<SDR_DQM_LDQM_BIT);
#endif
}
static void sdram_init_c()
{
	struct gpioreg * bank_port = (struct gpioreg *)&SDR_BANK_PORT;
	struct gpioreg * dqm_port = (struct gpioreg *)&SDR_DQM_PORT;
	struct gpioreg * address_lb_port = (struct gpioreg *)&SDR_ADDRESS_LB_PORT;
	struct gpioreg * address_port = (struct gpioreg *)&SDR_ADDRESS_PORT;
	struct gpioreg * data_port = (struct gpioreg *)&SDR_DATA_PORT;
	struct gpioreg * control_port = (struct gpioreg *)&SDR_CONTROL_PORT;
	struct gpioreg * cke_port = (struct gpioreg *)&SDR_CKE_PORT;
	struct ocreg * ocr_reg = (struct ocreg *)&SDR_OCR;

// make f13 a ground pin?
address_lb_port->latclr = (1<<13)|(1<<8);
address_lb_port->trisclr = (1<<13)|(1<<8);

	address_lb_port->trisclr = ADDRESS_LB_MASK;	
	address_port->trisclr = ADDRESS_MASK;
	
	/* AD1PCFGSET = 0xFFFF; */
	bank_port->trisclr = BANK_ALL_MASK;

#ifdef SDR_DQM_UDQM_BIT
	dqm_port->latset = (1<<SDR_DQM_UDQM_BIT);
#endif
	dqm_port->latclr = (1<<SDR_DQM_LDQM_BIT);
	dqm_port->trisclr = SDR_DQM_MASK;

    	/* All address lines low */
	address_lb_port->latclr = ADDRESS_LB_MASK;
	address_port->latclr = ADDRESS_MASK;
        
	bank_port->latclr = BANK_ALL_MASK;

	/* Initialize data lines */
	data_port->trisset = 0xff;

	/* Initialize SDRAM control lines */
	control_port->trisclr = CONTROL_ALL_MASK;

	/* Command Inhibit */
	control_port->latset = CONTROL_ALL_MASK;

	/* Initialize CKE line */
    	cke_port->trisclr = (1<<SDR_CKE_BIT);

	/* CKE low */
	cke_port->latclr = (1<<SDR_CKE_BIT);

#ifdef SDRAM_FPGA_DIR_SUPPORT
	struct gpioreg * data_dir_port = (struct gpioreg *)&SDR_DATA_DIR_PORT;
	data_dir_port->latset = (1<<SDR_DATA_DIR_BIT); 
	data_dir_port->trisclr = (1<<SDR_DATA_DIR_BIT);
#endif

	/* SDRAM clock output */
    
	/* Initialize Timer2 */
	T2CON = 0;
	TMR2 = 0;
	PR2 = 3;	
	T2CONSET = 0x8000;

	/* Initialize OC device */
	ocr_reg->con = 0;
	ocr_reg->rs = 1;	
	ocr_reg->r = 3;
	ocr_reg->con = 0x8005;	

	/* Clock output starts here */

	/* SD-RAM initialization delay */
	unsigned cc_start, cc_now; 
	asm volatile("mfc0 %0, $9" : "=r" (cc_start));
	do
	{
		asm volatile("mfc0 %0, $9" : "=r" (cc_now));
	} while( cc_now - cc_start < 500 );

	/* CKE high */
	cke_port->latset = (1<<SDR_CKE_BIT);

	/* Delay some more */

	asm volatile("mfc0 %0, $9" : "=r" (cc_start));
	do
	{
		asm volatile("mfc0 %0, $9" : "=r" (cc_now));
	} while( cc_now - cc_start < 3000 );

	/* finish up in assembly routine */
	sdram_init();
}


//static void sdram_output_column( unsigned column )
static void sdram_output_addr( unsigned addr )
{
	struct gpioreg * address_lb_port = (struct gpioreg *)&SDR_ADDRESS_LB_PORT;
	struct gpioreg * address_port = (struct gpioreg *)&SDR_ADDRESS_PORT;
	address_lb_port->lat = (address_lb_port->lat & ~ADDRESS_LB_MASK) | ((addr & 0x7) << SDR_ADDRESS_LB_A0_BIT);
	address_port->lat = (address_port->lat & ~ADDRESS_MASK) 
		| ((addr & (ADDRESS_MASK<<(3-SDR_ADDRESS_A3_BIT))) >> (3-SDR_ADDRESS_A3_BIT));
}

static void sdram_active_c( unsigned row_address )
{
	sdram_output_addr( row_address );
	sdram_active();
}

static void sdram_write_c(uint16_t coladdr, uint64_t val)
{
	sdram_output_addr( coladdr );
	sdram_write( val );
}

static uint64_t sdram_read_c(uint16_t coladdr)
{
	sdram_output_addr( coladdr );
	return sdram_read();
}

static void 
read_chunk_from_sdram( uint64_t* dest, unsigned int blockNumber )
{
	int startColumn = ( ( blockNumber & ( BLOCKS_PER_ROW - 1 ) ) * CHUNK_SIZE ); // / RAM_BURST_COUNT;
	int rowAndBank = blockNumber / BLOCKS_PER_ROW;
	int row = rowAndBank & ( RAM_ROWS - 1 );
	int bank = rowAndBank / RAM_ROWS;
	int sbank = bank / 4;
	bank  = bank & 0b011;
	int col = startColumn;

	sdram_upperlowerbyte( sbank );

	while( col < startColumn + CHUNK_SIZE/*/RAM_BURST_COUNT*/ ) {
		int x = mips_intr_disable();
		sdram_wake();
		sdram_bank_c(bank);
		sdram_active_c(row);
		int i;
		for( i = 0; i < RAM_BURST_GROUP_COUNT; i++ ) {
			*dest++ = sdram_read_c( col );
			col += RAM_BURST_COUNT;
		}
		sdram_precharge();
		sdram_precharge_all();
		sdram_sleep();

		mips_intr_restore (x);

		asm volatile ("nop");
	} 
}

static void 
write_chunk_to_sdram( uint64_t* src, unsigned int blockNumber )
{
	int startColumn = ( ( blockNumber & ( BLOCKS_PER_ROW - 1 ) ) * CHUNK_SIZE ); /// RAM_BURST_COUNT;
	int rowAndBank = blockNumber / BLOCKS_PER_ROW;
	int row = rowAndBank & ( RAM_ROWS - 1 );
	int bank = rowAndBank / RAM_ROWS;
	int sbank = bank / 4;
	bank  = bank & 0b011;
  
	sdram_upperlowerbyte( sbank );

	int col = startColumn;
	while( col < startColumn + CHUNK_SIZE /*/RAM_BURST_COUNT*/ ) {
		int x = mips_intr_disable();

		sdram_wake();
		sdram_bank_c(bank);
		sdram_active_c(row);
		int i;
		for( i = 0; i < RAM_BURST_GROUP_COUNT; i++ ) {
			sdram_write_c( col, *src++ );
			col += RAM_BURST_COUNT;
		}
		sdram_precharge();
		sdram_precharge_all();
		sdram_sleep();

		mips_intr_restore (x);
      
		asm volatile ("nop");
	} 
}


/*
 * Read a block of data.
 */
int
sdramp_read(int unit, unsigned blockno, char* data, unsigned nbytes)
{
	blockno = blockno * (DEV_BSIZE/CHUNK_SIZE);

	while( nbytes >= CHUNK_SIZE ) {
		read_chunk_from_sdram( (uint64_t*) swaptemp, blockno );
		bcopy( swaptemp, data, CHUNK_SIZE );
		data += CHUNK_SIZE;
		blockno += 1;
		nbytes -= CHUNK_SIZE;
	}

	if( nbytes ) {
		read_chunk_from_sdram( (uint64_t*) swaptemp, blockno );
		bcopy( swaptemp, data, nbytes );
	}

	return 1;
}

/*
 * Write a block of data.
 */
int
sdramp_write (int unit, unsigned blockno, char *data, unsigned nbytes)
{
	blockno = blockno * (DEV_BSIZE/CHUNK_SIZE);

	while( nbytes >= CHUNK_SIZE ) {
		bcopy( data, swaptemp, CHUNK_SIZE );
        int x = mips_intr_disable();
		write_chunk_to_sdram( (uint64_t*) swaptemp, blockno );
        mips_intr_restore(x);
		data += CHUNK_SIZE;
		blockno += 1;
		nbytes -= CHUNK_SIZE;
	}
	if( nbytes ) {
		read_chunk_from_sdram( (uint64_t*) swaptemp, blockno );
		bcopy( data, swaptemp, nbytes );
        int x = mips_intr_disable();
		write_chunk_to_sdram( (uint64_t*) swaptemp, blockno );
        mips_intr_restore(x);
	}
	return 1;
}

void sdramp_preinit (int unit)
{
        printf("sdramp_preinit\n");
        
	int x = mips_intr_disable();
	struct buf *bp;
	sdram_init_c();
	mips_intr_restore(x);

        bp = prepartition_device("sdramp0"/*,sdramp_size(0)*/);
        if(bp)
        {
                sdramp_write (0, 0, bp->b_addr, 512);
                brelse(bp);
        }

}

int sdramp_open(int unit, int flag, int mode)
{
	return 0;	
}

int sdramp_size(int unit)
{
	return (1<<SDR_ADDRESS_LINES) / 2 * 4 * SDR_DATA_BYTES;
}

#if 0
// FIXME - this does not supply any more than
// is currently used by the rdisk functions. It does
// not build a completely or valid mbr.

void build_ramdisk_mbr( struct mbr* m )
{
	bzero( m, 512 );
#ifdef RAMDISK_MBR_PARTS
	RAMDISK_MBR_PARTS;
#endif
}
#endif
