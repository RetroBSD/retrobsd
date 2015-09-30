/*
 * Driver for external SDRAM-based swap device.
 *
 * See sdram.S for information on interface to sdram
 *
 * This code could use a bit of optimization.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/dk.h>
#include <sys/ioctl.h>
#include <sys/disk.h>
#include <machine/sdram.h>
#include <machine/sdramp.h>
#include <sys/kconfig.h>

/*
 * See rd_sdramp_config.h for sdramp port/pin configuration
 */
#include <machine/sdramp_config.h>

int sdramp_dkindex = -1;                /* Statistics slot number */

/*
 * physical specs of SDRAM chip
 */
#define RAM_COLS                512

#if SDR_ADDRESS_LINES == 13
#define RAM_ROWS                (4096*2)
#elif SDR_ADDRESS_LINES == 12
#define RAM_ROWS                4096
#elif SDR_ADDRESS_LINES == 11
#define RAM_ROWS                2048
#else
#error Invalid Configuration - SDR_ADDRESS_LINES
#endif

#define RAM_BANKS               4

/*
 * RAM_BURST_COUNT MUST be match the number of bytes
 * read/written by each call to sdram_read/sdram_write
 */
#define RAM_BURST_COUNT         8

/*
 * CHUNK_SIZE number of bytes in each "chunk"
 */
#define CHUNK_SIZE              32

#define RAM_BURST_GROUP_COUNT   4

#define BLOCKS_PER_ROW          (RAM_COLS / CHUNK_SIZE)

/*
 * Size of the whole disk in kbytes.
 */
#define SDR_TOTAL_KBYTES    ((1<<SDR_ADDRESS_LINES) / 2 * 4 * SDR_DATA_BYTES)

static char swaptemp[CHUNK_SIZE];

/*
 * Used to specify partition table for ramdisk from Makefile
 */
#define RAMDISK_PARTSPEC(n,t,s,l) \
    m->partitions[n].type = t; \
    m->partitions[n].lbastart = s; \
    m->partitions[n].lbalength = l;

// FIXME - FOLLOWING shared with gpio.c - needs to be made common

struct ocreg {
    volatile unsigned con;      /* ? */
    volatile unsigned conclr;
    volatile unsigned conset;
    volatile unsigned coninv;
    volatile unsigned r;        /* ? */
    volatile unsigned rclr;
    volatile unsigned rset;
    volatile unsigned rinv;
    volatile unsigned rs;       /* ? */
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

static void sdram_upperlowerbyte(unsigned bit)
{
    struct gpioreg * dqm_port = (struct gpioreg *)&SDR_DQM_PORT;
#ifdef SDR_DQM_UDQM_BIT

    if (bit == 0) {
        dqm_port->latset = (1<<SDR_DQM_UDQM_BIT);
        dqm_port->latclr = (1<<SDR_DQM_LDQM_BIT);
    } else {
        dqm_port->latset = (1<<SDR_DQM_LDQM_BIT);
        dqm_port->latclr = (1<<SDR_DQM_UDQM_BIT);
    }

//  dqm_port->latset = (1<<SDR_DQM_UDQM_BIT);
//  dqm_port->latclr = (1<<SDR_DQM_LDQM_BIT);
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
    address_lb_port->latclr = (1<<13) | (1<<8);
    address_lb_port->trisclr = (1<<13) | (1<<8);

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
    do {
        asm volatile("mfc0 %0, $9" : "=r" (cc_now));
    } while (cc_now - cc_start < 500);

    /* CKE high */
    cke_port->latset = (1<<SDR_CKE_BIT);

    /* Delay some more */

    asm volatile("mfc0 %0, $9" : "=r" (cc_start));
    do {
        asm volatile("mfc0 %0, $9" : "=r" (cc_now));
    } while (cc_now - cc_start < 3000);

    /* finish up in assembly routine */
    sdram_init();
}

static void sdram_output_addr(unsigned addr)
{
    struct gpioreg * address_lb_port = (struct gpioreg *)&SDR_ADDRESS_LB_PORT;
    struct gpioreg * address_port = (struct gpioreg *)&SDR_ADDRESS_PORT;
    address_lb_port->lat = (address_lb_port->lat & ~ADDRESS_LB_MASK) | ((addr & 0x7) << SDR_ADDRESS_LB_A0_BIT);
    address_port->lat = (address_port->lat & ~ADDRESS_MASK)
        | ((addr & (ADDRESS_MASK<<(3-SDR_ADDRESS_A3_BIT))) >> (3-SDR_ADDRESS_A3_BIT));
}

static void sdram_active_c(unsigned row_address)
{
    sdram_output_addr(row_address);
    sdram_active();
}

static void sdram_write_c(uint16_t coladdr, uint64_t val)
{
    sdram_output_addr(coladdr);
    sdram_write(val);
}

static uint64_t sdram_read_c(uint16_t coladdr)
{
    sdram_output_addr(coladdr);
    return sdram_read();
}

static void
read_chunk_from_sdram(uint64_t* dest, unsigned int blockNumber)
{
    int startColumn = (blockNumber & (BLOCKS_PER_ROW - 1)) * CHUNK_SIZE; // / RAM_BURST_COUNT;
    int rowAndBank = blockNumber / BLOCKS_PER_ROW;
    int row = rowAndBank & (RAM_ROWS - 1);
    int bank = rowAndBank / RAM_ROWS;
    int sbank = bank / 4;
    bank  = bank & 0b011;
    int col = startColumn;

    sdram_upperlowerbyte(sbank);

    while (col < startColumn + CHUNK_SIZE/*/RAM_BURST_COUNT*/) {
        int x = mips_intr_disable();
        sdram_wake();
        sdram_bank_c(bank);
        sdram_active_c(row);
        int i;
        for (i = 0; i < RAM_BURST_GROUP_COUNT; i++) {
            *dest++ = sdram_read_c(col);
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
write_chunk_to_sdram(uint64_t* src, unsigned int blockNumber)
{
    int startColumn = (blockNumber & (BLOCKS_PER_ROW - 1)) * CHUNK_SIZE; /// RAM_BURST_COUNT;
    int rowAndBank = blockNumber / BLOCKS_PER_ROW;
    int row = rowAndBank & (RAM_ROWS - 1);
    int bank = rowAndBank / RAM_ROWS;
    int sbank = bank / 4;
    bank  = bank & 0b011;

    sdram_upperlowerbyte(sbank);

    int col = startColumn;
    while (col < startColumn + CHUNK_SIZE /*/RAM_BURST_COUNT*/) {
        int x = mips_intr_disable();

        sdram_wake();
        sdram_bank_c(bank);
        sdram_active_c(row);
        int i;
        for (i = 0; i < RAM_BURST_GROUP_COUNT; i++) {
            sdram_write_c(col, *src++);
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
static int
sdramp_read(unsigned blockno, char* data, unsigned nbytes)
{
    blockno = blockno * (DEV_BSIZE/CHUNK_SIZE);

    while (nbytes >= CHUNK_SIZE) {
        read_chunk_from_sdram((uint64_t*) swaptemp, blockno);
        bcopy(swaptemp, data, CHUNK_SIZE);
        data += CHUNK_SIZE;
        blockno += 1;
        nbytes -= CHUNK_SIZE;
    }

    if (nbytes) {
        read_chunk_from_sdram((uint64_t*) swaptemp, blockno);
        bcopy(swaptemp, data, nbytes);
    }
    return 1;
}

/*
 * Write a block of data.
 */
static int
sdramp_write (unsigned blockno, char *data, unsigned nbytes)
{
    blockno = blockno * (DEV_BSIZE/CHUNK_SIZE);

    while (nbytes >= CHUNK_SIZE) {
        bcopy(data, swaptemp, CHUNK_SIZE);
        int x = mips_intr_disable();
        write_chunk_to_sdram((uint64_t*) swaptemp, blockno);
        mips_intr_restore(x);
        data += CHUNK_SIZE;
        blockno += 1;
        nbytes -= CHUNK_SIZE;
    }
    if (nbytes) {
        read_chunk_from_sdram((uint64_t*) swaptemp, blockno);
        bcopy(data, swaptemp, nbytes);
        int x = mips_intr_disable();
        write_chunk_to_sdram((uint64_t*) swaptemp, blockno);
        mips_intr_restore(x);
    }
    return 1;
}

int sdramp_open(dev_t dev, int flag, int mode)
{
    return 0;
}

int sdramp_close(dev_t dev, int flag, int mode)
{
    return 0;
}

/*
 * Return a size of partition in kbytes.
 * The memory is divided into two partitions: A and B.
 * Size of partition B is specified in the kernel config file
 * as option SDR_SWAP_KBYTES.
 */
daddr_t sdramp_size(dev_t dev)
{
    switch (minor(dev)) {
    case 0:
        /* Whole disk. */
        return SDR_TOTAL_KBYTES;
    case 1:
        /* Partition A: filesystem. */
        return SDR_TOTAL_KBYTES - SDR_SWAP_KBYTES;
    case 2:
    default:
        /* Partition B: swap space. */
        return SDR_SWAP_KBYTES;
    }
}

void sdramp_strategy(struct buf *bp)
{
    int offset = bp->b_blkno;
    long nblk = btod(bp->b_bcount);
    int part_offset, part_size, s;

    /* Compute partition size and offset. */
    part_size = sdramp_size(bp->b_dev);
    if (minor(bp->b_dev) < 2) {
        /* Partition A or a whole disk. */
        part_offset = 0;
    } else {
        /* Partition B: swap space. */
        part_offset = SDR_TOTAL_KBYTES - part_size;
    }

    /*
     * Determine the size of the transfer, and make sure it is
     * within the boundaries of the partition.
     */
    offset += part_offset;
    if (bp->b_blkno + nblk > part_size) {
        /* if exactly at end of partition, return an EOF */
        if (bp->b_blkno == part_size) {
            bp->b_resid = bp->b_bcount;
            biodone(bp);
            return;
        }
        /* or truncate if part of it fits */
        nblk = part_size - bp->b_blkno;
        if (nblk <= 0) {
            bp->b_error = EINVAL;
            bp->b_flags |= B_ERROR;
            biodone(bp);
            return;
        }
        bp->b_bcount = nblk << DEV_BSHIFT;
    }

    if (bp->b_dev == swapdev) {
        led_control(LED_SWAP, 1);
    } else {
        led_control(LED_DISK, 1);
    }

    s = splbio();
#ifdef UCB_METER
    if (sdramp_dkindex >= 0) {
        dk_busy |= 1 << sdramp_dkindex;
        dk_xfer[sdramp_dkindex]++;
        dk_bytes[sdramp_dkindex] += bp->b_bcount;
    }
#endif

    if (bp->b_flags & B_READ) {
        sdramp_read(offset, bp->b_addr, bp->b_bcount);
    } else {
        sdramp_write(offset, bp->b_addr, bp->b_bcount);
    }

    biodone(bp);
    if (bp->b_dev == swapdev) {
        led_control(LED_SWAP, 0);
    } else {
        led_control(LED_DISK, 0);
    }
#ifdef UCB_METER
    if (sdramp_dkindex >= 0)
        dk_busy &= ~(1 << sdramp_dkindex);
#endif
    splx(s);
}

int sdramp_ioctl(dev_t dev, u_int cmd, caddr_t addr, int flag)
{
    int error = 0;

    switch (cmd) {

    case DIOCGETMEDIASIZE:
        /* Get disk size in kbytes. */
        *(int*) addr = sdramp_size(dev);
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
sdrampprobe(config)
    struct conf_device *config;
{
    /* Only one device unit is supported. */
    if (config->dev_unit != 0)
        return 0;

    printf("dr0: total %u kbytes, swap space %u kbytes\n",
        SDR_TOTAL_KBYTES, SDR_SWAP_KBYTES);

    int x = mips_intr_disable();
    sdram_init_c();
    mips_intr_restore(x);

#ifdef UCB_METER
    dk_alloc(&sdramp_dkindex, 1, "dr0");
#endif
    return 1;
}

struct driver sdrampdriver = {
    "sdramp", sdrampprobe,
};
