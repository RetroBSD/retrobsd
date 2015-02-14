/*
 * Measure a speed of file read/write operations.
 *
 * Copyright (c) 2015, Serge Vakulenko
 *
 * Permission to use, copy, modify, and/or distribute this
 * software for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice and this permission
 * notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_BLOCK_SZ    64      /* kbytes */
#define MAX_DATA_SZ     1024    /* Mbytes */

const char version[] = "1.0";
const char copyright[] = "Copyright (C) 2015 Serge Vakulenko";

char *progname;
int verbose;
char block[MAX_BLOCK_SZ*1024];

/*
 * Get current time in microseconds.
 */
unsigned current_msec()
{
    struct timeval t;

    gettimeofday(&t, 0);
    return t.tv_sec * 1000 + t.tv_usec / 1000;
}

unsigned elapsed_msec(unsigned t0)
{
    struct timeval t1;
    unsigned msec;

    gettimeofday (&t1, 0);
    msec = t1.tv_sec * 1000 + t1.tv_usec / 1000;
    msec -= t0;
    if (msec < 1)
        msec = 1;
    return msec;
}

void usage()
{
    fprintf(stderr, "Disk speed test, Version %s, %s\n", version, copyright);
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "    %s [-v] [-b blocksz] [-m datasz] [filename]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "    -v    verbose mode\n");
    fprintf(stderr, "    -b #  block size in kbytes, default 4\n");
    fprintf(stderr, "    -m #  data size in Mbytes, default 8\n");
    exit(-1);
}

int main(int argc, char **argv)
{
    int blocksize_kbytes = 4;
    int datasize_mbytes = 8;
    int nbytes, fd, n;
    char *filename = 0;
    unsigned t0, msec;

    progname = *argv;
    for (;;) {
        switch (getopt(argc, argv, "vb:m:")) {
        case EOF:
            break;
        case 'v':
            ++verbose;
            continue;
        case 'b':
            blocksize_kbytes = strtol(optarg, 0, 0);
            continue;
        case 'm':
            datasize_mbytes = strtol(optarg, 0, 0);
            continue;
        default:
            usage();
        }
        break;
    }
    argc -= optind;
    argv += optind;

    if (argc > 1)
        usage();

    if (argc == 1)
        filename = argv[0];

    /*
     * Verify parameters.
     */
    if (blocksize_kbytes < 1 || blocksize_kbytes > MAX_BLOCK_SZ) {
        fprintf(stderr, "Bad block size = %d kbytes.\n", blocksize_kbytes);
        fprintf(stderr, "Valid range is 1...%d kbytes.\n", MAX_BLOCK_SZ);
        exit(-1);
    }
    if (datasize_mbytes < 1 || datasize_mbytes > MAX_DATA_SZ) {
        fprintf(stderr, "Bad data size = %d Mbytes.\n", datasize_mbytes);
        fprintf(stderr, "Valid range is 1...%d Mbytes.\n", MAX_DATA_SZ);
        exit(-1);
    }
    if (filename)
        printf("File name: %s\n", filename);
    else
        filename = "diskspeed.data";
    if (access(filename, 0) >= 0) {
        fprintf(stderr, "File '%s' already exists: cannot overwrite.\n",
            filename);
        fprintf(stderr, "Please, delete the file manually.\n");
        exit(-1);
    }
    printf("Testing %d-kbyte block size.\n", blocksize_kbytes);

    /*
     * Fill buffer with some data.
     */
    for (n=0; n<sizeof(block); n++) {
        block[n] = ~n;
    }

    /*
     * Open the file.
     */
    fd = open(filename, O_RDWR | O_CREAT, 0664);
    if (fd < 0) {
        fprintf(stderr, "Cannot create file '%s'.\n", filename);
        exit(-1);
    }
    if (verbose)
        printf("Created file '%s'.\n", filename);

    /*
     * Write data to file.
     */
    sync();
    usleep(200000);
    sync();
    usleep(200000);
    t0 = current_msec();
    nbytes = blocksize_kbytes * 1024;
    for (n=0; n<datasize_mbytes*1024/blocksize_kbytes; n++) {
        if (write(fd, block, nbytes) != nbytes) {
            fprintf(stderr, "Write error at block %d.\n", n);
            exit(-1);
        }
    }
    msec = elapsed_msec(t0);
    printf ("Write speed: %u Mbytes in %u.%03u seconds = %u kbytes/sec\n",
        datasize_mbytes, msec/1000, msec%1000,
        datasize_mbytes*1024000U / msec);

    /*
     * Read data from file.
     */
    sync();
    usleep(200000);
    sync();
    usleep(200000);
    lseek(fd, 0, SEEK_SET);
    t0 = current_msec();
    for (n=0; n<datasize_mbytes*1024/blocksize_kbytes; n++) {
        if (read(fd, block, nbytes) != nbytes) {
            fprintf(stderr, "Read error at block %d.\n", n);
            exit(-1);
        }
    }
    msec = elapsed_msec(t0);
    printf (" Read speed: %u Mbytes in %u.%03u seconds = %u kbytes/sec\n",
        datasize_mbytes, msec/1000, msec%1000,
        datasize_mbytes*1024000U / msec);

    close(fd);
    unlink(filename);
    if (verbose)
        printf("File '%s' deleted.\n", filename);
    return 0;
}
