/*
 * Disk testing utility.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define BLOCKSZ     1024

char *progname;
int verbose;
int fd;

void block_write (unsigned bn, unsigned char *data)
{
    unsigned addr = bn * BLOCKSZ;
    int ret, i;

    if (verbose) {
        printf ("Write block to address %06X.\n", addr);
	printf ("    %02x", data[0]);
	for (i=1; i<BLOCKSZ; i++)
            printf ("-%02x", data[i]);
	printf ("\n");
    }
    ret = lseek (fd, addr, 0);
    if (ret != addr) {
        printf ("Seek error at %06X: result %06X, expected %06X.\n",
            addr, ret, addr);
        exit (-1);
    }
    ret = write (fd, data, BLOCKSZ);
    if (ret != BLOCKSZ) {
        printf ("Write error at %06X: %s\n", addr, strerror(ret));
        exit (-1);
    }
}

void block_read (unsigned bn, unsigned char *data)
{
    unsigned addr = bn * BLOCKSZ;
    int ret, i;

    if (verbose)
        printf ("Read block from address %06X.\n", addr);

    ret = lseek (fd, addr, 0);
    if (ret != addr) {
        printf ("Seek error: result %06X, expected %06X.\n", ret, addr);
        exit (-1);
    }
    ret = read (fd, data, BLOCKSZ);
    if (ret != BLOCKSZ) {
        printf ("Read error at %06X: %s\n", addr, strerror(ret));
        exit (-1);
    }
    if (verbose) {
	printf ("    %02x", data[0]);
	for (i=1; i<BLOCKSZ; i++)
            printf ("-%02x", data[i]);
	printf ("\n");
    }
}

/*
 * Fill an array with pattern data.
 * When alt=~0, use counter starting from given value.
 * Otherwise, use pattern-alt-pattern-alt ans so on.
 */
void data_fill (unsigned char *data, unsigned pattern, unsigned alt)
{
    int i;

    for (i=0; i<BLOCKSZ; ++i) {
        if (alt == ~0)
            data[i] = pattern++;
        else
            data[i] = (i & 1) ? alt : pattern;
    }
}

void data_check (unsigned char *data, unsigned char *rdata)
{
    int i;

    for (i=0; i<BLOCKSZ; ++i) {
        if (rdata[i] != data[i])
            printf ("Data error: offset %d written %02X read %02X.\n",
                i, data[i], rdata[i]);
    }
}

void test_alternate (unsigned blocknum)
{
    unsigned char data [BLOCKSZ];
    unsigned char rdata [BLOCKSZ];

    printf ("Testing block %u at address %06X.\n",
        blocknum, blocknum * BLOCKSZ);

    data_fill (data, 0x55, 0xAA);
    block_write (blocknum, data);
    block_read (blocknum, rdata);
    data_check (data, rdata);

    data_fill (data, 0xAA, 0x55);
    block_write (blocknum, data);
    block_read (blocknum, rdata);
    data_check (data, rdata);

    printf ("Done.\n");
}

void test_counter (unsigned blocknum)
{
    unsigned char data [BLOCKSZ];
    unsigned char rdata [BLOCKSZ];

    printf ("Testing block %u at address %06X.\n",
        blocknum, blocknum * BLOCKSZ);

    data_fill (data, 0, ~0);
    block_write (blocknum, data);
    block_read (blocknum, rdata);
    data_check (data, rdata);

    printf ("Done.\n");
}

void test_blocks (unsigned blocknum)
{
    unsigned char data [BLOCKSZ];
    unsigned char rdata [BLOCKSZ];
    unsigned bn;
    int i;

    /* Pass 1: write data. */
    for (i=0; ; i++) {
        /* Use block numbers 0, 1, 2, 4, 8, 16... blocknum (inclusive). */
        if (i == 0)
            bn = 0;
        else
            bn = 1 << (i - 1);
        if (bn > blocknum)
            bn = blocknum;

        printf ("Writing block %u at address %06X.\n",
            bn, bn * BLOCKSZ);

        /* Use counter pattern, different for every next block. */
        data_fill (data, i, ~0);
        block_write (bn, data);

        if (bn == blocknum)
            break;
    }

    /* Pass 2: read and check. */
    for (i=0; ; i++) {
        /* Use block numbers 0, 1, 2, 4, 8, 16... blocknum (inclusive). */
        if (i == 0)
            bn = 0;
        else
            bn = 1 << (i - 1);
        if (bn > blocknum)
            bn = blocknum;

        printf ("Reading block %u at address %06X.\n",
            bn, bn * BLOCKSZ);

        /* Use counter pattern, different for every next block. */
        data_fill (data, i, ~0);
        block_read (bn, rdata);
        data_check (data, rdata);

        if (bn == blocknum)
            break;
    }
    printf ("Done.\n");
}

void fill_blocks (unsigned blocknum, unsigned char pattern)
{
    unsigned char data [BLOCKSZ];
    unsigned char rdata [BLOCKSZ];
    unsigned bn;

    printf ("Testing blocks 0-%u with byte value %02X.\n",
        blocknum, pattern);
    for (bn=0; bn<=blocknum; bn++) {

        /* Use counter pattern, different for every next block. */
        data_fill (data, pattern, pattern);
        block_write (bn, data);
        block_read (bn, rdata);
        data_check (data, rdata);

        if (bn == blocknum)
            break;
    }
    printf ("Done.\n");
}

void usage ()
{
    fprintf (stderr, "Disk testing utility.\n");
    fprintf (stderr, "Usage:\n");
    fprintf (stderr, "    %s [options] device [blocknum]\n", progname);
    fprintf (stderr, "Options:\n");
    fprintf (stderr, "    -a              -- test a block with alternate pattern 55/AA\n");
    fprintf (stderr, "    -c              -- test a block with counter pattern 0-1-2...FF\n");
    fprintf (stderr, "    -b              -- test blocks 0-1-2-4-8...blocknum\n");
    fprintf (stderr, "    -f pattern      -- fill blocks 0...blocknum with given byte value\n");
    fprintf (stderr, "    -v              -- verbose mode\n");
    exit (-1);
}

int main (int argc, char **argv)
{
    int aflag = 0, bflag = 0, cflag = 0, fflag = 0;
    unsigned blocknum = 0, pattern = 0;

    progname = *argv;
    for (;;) {
        switch (getopt (argc, argv, "abcf:v")) {
        case EOF:
            break;
        case 'a':
            aflag = 1;
            continue;
        case 'c':
            cflag = 1;
            continue;
        case 'b':
            bflag = 1;
            continue;
        case 'f':
            fflag = 1;
            pattern = strtoul (optarg, 0, 0);
            continue;
        case 'v':
            verbose = 1;
            continue;
        default:
            usage ();
        }
        break;
    }
    argc -= optind;
    argv += optind;
    if (argc < 1 || argc > 2 || aflag + bflag + cflag + fflag == 0)
        usage ();

    fd = open (argv[0], O_RDWR);
    if (fd < 0) {
        perror (argv[0]);
        return -1;
    }
    if (argc > 1)
        blocknum = strtoul (argv[1], 0, 0);

    if (aflag)
        test_alternate (blocknum);

    if (cflag)
        test_counter (blocknum);

    if (bflag)
        test_blocks (blocknum);

    if (fflag)
        fill_blocks (blocknum, pattern);

    return 0;
}
