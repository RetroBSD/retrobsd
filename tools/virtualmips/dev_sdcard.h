/*
 * SecureDigital flash card.
 *
 * Copyright (C) 2011 Serge Vakulenko <serge@vak.ru>
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */
#ifndef __DEV_SD__
#define __DEV_SD__

/* SD card private data */
struct sdcard {
    char *name;                         /* Device name */
    unsigned kbytes;                    /* Disk size */
    int unit;                           /* Index (sd0 or sd1) */
    int fd;                             /* Image file */
    int select;                         /* Selected */
    int read_multiple;                  /* Read-multiple mode */
    unsigned blen;                      /* Block length */
    unsigned wbecnt;                    /* Write block erase count */
    unsigned offset;                    /* Read/write offset */
    unsigned count;                     /* Byte count */
    unsigned limit;                     /* Reply length */
    unsigned char buf [1024 + 16];
};
typedef struct sdcard sdcard_t;

int dev_sdcard_init (sdcard_t *d, char *devname, unsigned mbytes, char *filename);
void dev_sdcard_reset (cpu_mips_t *cpu);
void dev_sdcard_select (cpu_mips_t *cpu, int unit, int on);
unsigned dev_sdcard_io (cpu_mips_t *cpu, unsigned data);

#endif
