/*
 * RAM-based disk for swap.
 *
 * Copyright (C) 2011 Igor Mokos
 * Copyright (C) 2011 Serge Vakulenko <serge@vak.ru>
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */
#ifndef __DEV_SWAP__
#define __DEV_SWAP__

#define SWAP_BYTES  (2*1024*1024)       /* Disk size */

/* Swap device private data */
struct swap {
    char *name;                         /* Device name */
    int rd;                             /* RD signal */
    int wr;                             /* WR signal */
    int ldaddr;                         /* LDADDR signal */
    unsigned char data;                 /* Latched byte */
    unsigned offset;                    /* Read/write offset */
    unsigned char buf [SWAP_BYTES];     /* Stored data */
};
typedef struct swap swap_t;

int dev_swap_init (swap_t *d, char *devname);
void dev_swap_reset (cpu_mips_t *cpu);
void dev_swap_rd (cpu_mips_t *cpu, int on);
void dev_swap_wr (cpu_mips_t *cpu, int on);
void dev_swap_ldaddr (cpu_mips_t *cpu, int on);
unsigned dev_swap_io (cpu_mips_t *cpu, unsigned char data, unsigned char mask);

#endif
