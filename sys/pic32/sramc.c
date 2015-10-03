/*
 * Driver for external SRAM-CPLD based Swap and Filesystem devices
 *
 * This version is for 8MB RAMDISK v.1.1 and compatible
 * Pito 7.4.2014 - PIC32MX PMP bus version
 * Under by retrobsd.org used Licence
 * No warranties of any kind
 *
 * Interface:
 *  PMD<7:0> - connected to PMP data bus
 *  PMRD      - fetch a byte from memory to data<7:0>, increment address, PMRD
 *  PMWR      - write a byte data[7:0] to memory, increment address, PMWR
 *  PMA0      - HIGH - write Address from data<3:0> in 6 steps: high nibble ... low nibble
 *            - LOW - write/read Data
 *
 * Signals PMRD, PMWR are active LOW and idle HIGH
 * Signal PMA0 is LOW when accessing RAM Data, and HIGH when accessing RAM Addresses
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/dk.h>
#include <sys/ioctl.h>
#include <sys/disk.h>
#include <sys/kconfig.h>
#include <machine/debug.h>
#include <machine/sramc.h>

int sramc_dkindex = -1;                 /* Statistics slot number */

/*
 * Size of RAM disk.
 */
#define SRAMC_TOTAL_KBYTES      8192    /* 4096 for 4MB ramdisk */

// Ramdisk v.1.1. wiring
// PMP         RAMDISK
// ===================
// PMD<D0-D7>  D0-D7
// PMRD        /RD
// PMWR        /WR
// PMA<0>      /DATA

// RD and WR pulses duration settings
// Minimal recommended settings, increase them when unstable
// No warranties of any kind
// for 120MHz clock, 70ns PSRAM, Ramdisk v.1.1.
#define ADR_PULSE  1
#define WR_PULSE   5
#define RD_PULSE   11

// for 80MHz clock, 70ns PSRAM, Ramdisk v.1.1.
//#define ADR_PULSE 1
//#define WR_PULSE 3
//#define RD_PULSE 8

// for 120MHz clock, 55ns SRAM
//#define ADR_PULSE 1
//#define WR_PULSE 3
//#define RD_PULSE 6

// for 80MHz clock, 55ns SRAM
//#define ADR_PULSE 1
//#define WR_PULSE 2
//#define RD_PULSE 4

typedef union {
    unsigned value;
    struct {
        unsigned nib1 : 4;  // lowest nibble
        unsigned nib2 : 4;
        unsigned nib3 : 4;
        unsigned nib4 : 4;
        unsigned nib5 : 4;
        unsigned nib6 : 4;
        unsigned nib7 : 4;
        unsigned nib8 : 4;  // highest nibble
    };
} nybbles;

/*
 * Load the 24 bit address to Ramdisk.
 *
 */
inline static void
dev_load_address(addr)
        unsigned addr;
{
    nybbles temp;
    temp.value = addr;

    while (PMMODE & 0x8000);            // Poll - if busy, wait

    PMADDR = 1;                         // set ADR mode (1) to write the Address

    PMMODE = 0b10<<8 | (ADR_PULSE<<2);  // full ADR speed

    PMDIN = temp.nib6;                  /* write 4 bits */

    while (PMMODE & 0x8000);            // Poll - if busy, wait
    PMDIN = temp.nib5;                  /* write 4 bits */

    while (PMMODE & 0x8000);            // Poll - if busy, wait
    PMDIN = temp.nib4;                  /* write 4 bits */

    while (PMMODE & 0x8000);            // Poll - if busy, wait
    PMDIN = temp.nib3;                  /* write 4 bits */

    while (PMMODE & 0x8000);            // Poll - if busy, wait
    PMDIN = temp.nib2;                  /* write 4 bits */

    while (PMMODE & 0x8000);            // Poll - if busy, wait
    PMDIN = temp.nib1;                  /* write 4 bits */
}

/*
 * Return a size of partition in kbytes.
 * The memory is divided into two partitions: A and B.
 * Size of partition B is specified in the kernel config file
 * as option SRAMC_SWAP_KBYTES.
 */
daddr_t sramc_size(dev_t dev)
{
    switch (minor(dev)) {
    case 0:
        /* Whole disk. */
        return SRAMC_TOTAL_KBYTES;
    case 1:
        /* Partition A: filesystem. */
        return SRAMC_TOTAL_KBYTES - SRAMC_SWAP_KBYTES;
    case 2:
    default:
        /* Partition B: swap space. */
        return SRAMC_SWAP_KBYTES;
    }
}

/*
 * Read a block of data.
 */
static int sramc_read(unsigned int blockno, char *data, unsigned int nbytes)
{
    int i;

    //DEBUG9("sramc%d: read block %u, length %u bytes, addr %p\n", major(dev), blockno, nbytes, data);

    dev_load_address(blockno * DEV_BSIZE);

    /* Read data. */

    while (PMMODE & 0x8000);            // Poll - if busy, wait

    PMADDR = 0;                         // set DATA mode (0)

    PMMODE = 0b10<<8 | (RD_PULSE<<2);   // read slowly

    PMDIN; // Read the PMDIN to clear previous data and latch new data

    for (i=0; i<nbytes; i++) {
        while (PMMODE & 0x8000);        // Poll - if busy, wait before reading
        *data++ = PMDIN;                /* read a byte of data */
    }
    return 1;
}

/*
 * Write a block of data.
 */
static int sramc_write(unsigned int blockno, char *data, unsigned int nbytes)
{
    unsigned i;

    //DEBUG9("sramc%d: write block %u, length %u bytes, addr %p\n", major(dev), blockno, nbytes, data);

    dev_load_address(blockno * DEV_BSIZE);

    /* Write data. */

    while (PMMODE & 0x8000);            // Poll - if busy, wait

    PMADDR = 0;                         // set DATA mode (0)

    PMMODE = 0b10<<8 | (WR_PULSE<<2);   // faster with write

    for (i=0; i<nbytes; i++) {
        while (PMMODE & 0x8000);        // Poll - if busy, wait
        PMDIN = *data++;                /* write a byte of data */
    }
    return 1;
}

/*
 * Init the disk.
 */
static void sramc_init()
{
    // Initialize PMP hardware

    PMCON = 0;                          // disable PMP
    asm volatile ("nop");               // Errata

    PMCON = 1<<9 | 1<<8;                // Enable RD and WR

    //        MODE  WAITB  WAITM  WAITE
    PMMODE = (2<<8) | 0 | (14<<2) | 0;  // Mode2 Master 8bit

    PMAEN = 1;                          // PMA<0>, use A0 only

    PMADDR = 0;                         // start with DATA mode

    PMCONSET = 1<<15;                   // PMP enabled
    asm volatile ("nop");

    // make a couple of dummy reads - it refreshes the cpld internals a little bit :)
    while (PMMODE & 0x8000);            // Poll - if busy, wait before reading
    PMDIN;                              /* read a byte of data */
    while (PMMODE & 0x8000);            // Poll - if busy, wait before reading
    PMDIN;                              /* read a byte of data */

    PMADDR = 1;                         // go with with ADDRESS mode now

    DEBUG3("sramc: init done\n");
}

/*
 * Open the disk.
 */
int sramc_open(dev_t dev, int flag, int mode)
{
    DEBUG3("sramc: open\n");
    return 0;
}

int sramc_close(dev_t dev, int flag, int mode)
{
    return 0;
}

void sramc_strategy(struct buf *bp)
{
    int offset = bp->b_blkno;
    long nblk = btod(bp->b_bcount);
    int part_offset, part_size, s;

    /* Compute partition size and offset. */
    part_size = sramc_size(bp->b_dev);
    if (minor(bp->b_dev) < 2) {
        /* Partition A or a whole disk. */
        part_offset = 0;
    } else {
        /* Partition B: swap space. */
        part_offset = SRAMC_TOTAL_KBYTES - part_size;
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
    if (sramc_dkindex >= 0) {
        dk_busy |= 1 << sramc_dkindex;
        dk_xfer[sramc_dkindex]++;
        dk_bytes[sramc_dkindex] += bp->b_bcount;
    }
#endif

    if (bp->b_flags & B_READ) {
        sramc_read(offset, bp->b_addr, bp->b_bcount);
    } else {
        sramc_write(offset, bp->b_addr, bp->b_bcount);
    }

    biodone(bp);
    if (bp->b_dev == swapdev) {
        led_control(LED_SWAP, 0);
    } else {
        led_control(LED_DISK, 0);
    }
#ifdef UCB_METER
    if (sramc_dkindex >= 0)
        dk_busy &= ~(1 << sramc_dkindex);
#endif
    splx(s);
}

int sramc_ioctl(dev_t dev, u_int cmd, caddr_t addr, int flag)
{
    int error = 0;

    switch (cmd) {

    case DIOCGETMEDIASIZE:
        /* Get disk size in kbytes. */
        *(int*) addr = sramc_size(dev);
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
sramcprobe(config)
    struct conf_device *config;
{
    /* Only one device unit is supported. */
    if (config->dev_unit != 0)
        return 0;

    printf("rc0: total %u kbytes, swap space %u kbytes\n",
        SRAMC_TOTAL_KBYTES, SRAMC_SWAP_KBYTES);

    sramc_init();

#ifdef UCB_METER
    dk_alloc(&sramc_dkindex, 1, "rc0");
#endif
    return 1;
}

struct driver rcdriver = {
    "sramc", sramcprobe,
};
