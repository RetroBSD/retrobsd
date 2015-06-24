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
#include "param.h"
#include "systm.h"
#include "buf.h"
#include "errno.h"
#include "dk.h"
#include "rdisk.h"

#include "debug.h"

int sw_dkn = -1;                /* Statistics slot number */

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
dev_load_address (addr)
        unsigned addr;
{
    nybbles temp;
    temp.value = addr;

    while(PMMODE & 0x8000);             // Poll - if busy, wait

    PMADDR = 1;                         // set ADR mode (1) to write the Address

    PMMODE = 0b10<<8 | (ADR_PULSE<<2);  // full ADR speed

    PMDIN = temp.nib6;                  /* write 4 bits */

    while(PMMODE & 0x8000);             // Poll - if busy, wait
    PMDIN = temp.nib5;                  /* write 4 bits */

    while(PMMODE & 0x8000);             // Poll - if busy, wait
    PMDIN = temp.nib4;                  /* write 4 bits */

    while(PMMODE & 0x8000);             // Poll - if busy, wait
    PMDIN = temp.nib3;                  /* write 4 bits */

    while(PMMODE & 0x8000);             // Poll - if busy, wait
    PMDIN = temp.nib2;                  /* write 4 bits */

    while(PMMODE & 0x8000);             // Poll - if busy, wait
    PMDIN = temp.nib1;                  /* write 4 bits */
}

/*
 * Get number of kBytes on the disk.
 * Return nonzero if successful.
 */
int sramc_size ( int unit )
{
    return 8192;                        // 4096 for 4MB ramdisk
}

/*
 * Read a block of data.
 */
inline int sramc_read (int unit, unsigned int blockno, char *data, unsigned int nbytes)
{
    int i;

    //DEBUG9("sramc%d: read block %u, length %u bytes, addr %p\n", major(dev), blockno, nbytes, data);

    dev_load_address (blockno * DEV_BSIZE);

    /* Read data. */

    while(PMMODE & 0x8000);             // Poll - if busy, wait

    PMADDR = 0;                         // set DATA mode (0)

    PMMODE = 0b10<<8 | (RD_PULSE<<2);   // read slowly

    PMDIN; // Read the PMDIN to clear previous data and latch new data

    for (i=0; i<nbytes; i++) {
        while(PMMODE & 0x8000);         // Poll - if busy, wait before reading
        *data++ = PMDIN;                /* read a byte of data */
    }
    return 1;
}

/*
 * Write a block of data.
 */
inline int sramc_write (int unit, unsigned int blockno, char *data, unsigned int nbytes)
{
    unsigned i;

    //DEBUG9("sramc%d: write block %u, length %u bytes, addr %p\n", major(dev), blockno, nbytes, data);

    dev_load_address (blockno * DEV_BSIZE);

    /* Write data. */

    while (PMMODE & 0x8000);            // Poll - if busy, wait

    PMADDR = 0;                         // set DATA mode (0)

    PMMODE = 0b10<<8 | (WR_PULSE<<2);   // faster with write

    for (i=0; i<nbytes; i++) {
        while(PMMODE & 0x8000);         // Poll - if busy, wait
        PMDIN = (*data++);              /* write a byte of data */
    }
    return 1;
}

/*
 * Init the disk.
 */
void sramc_init (int unit)
{
    struct buf *bp;

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
    while(PMMODE & 0x8000);             // Poll - if busy, wait before reading
    PMDIN;                              /* read a byte of data */
    while(PMMODE & 0x8000);             // Poll - if busy, wait before reading
    PMDIN;                              /* read a byte of data */

    PMADDR = 1;                         // go with with ADDRESS mode now

    DEBUG3("sramc%d: init done\n",unit);
    bp = prepartition_device("sramc0");
    if (bp) {
        sramc_write(0, 0, bp->b_addr, 512);
        brelse(bp);
    }
}

/*
 * Open the disk.
 */
int sramc_open (int unit)
{
    DEBUG3("sramc%d: open\n",unit);
    return 0;
}
