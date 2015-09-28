/*
 * Ioctl definitions for skeleton driver.
 *
 * Copyright (C) 2015 Serge Vakulenko
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#ifndef _SYS_DISK_H_
#define _SYS_DISK_H_

/*
 * IBM PC compatible partition table.
 */
#define MAXPARTITIONS   4

struct diskpart {                   /* the partition table */
    u_char      dp_status;          /* active (bootable) flag */
#define DP_ACTIVE 0x80
    u_char      dp_start_chs[3];    /* ignored */
    u_char      dp_type;            /* type of partition */
    u_char      dp_end_chs[3];      /* ignored */
    u_int       dp_offset;          /* starting sector */
    u_int       dp_nsectors;        /* number of sectors in partition */
};

/*
 * Partition types.
 */
#define PTYPE_UNUSED    0           /* unused */
#define PTYPE_BSDFFS    0xb7        /* 4.2BSD fast file system */
#define PTYPE_SWAP      0xb8        /* swap */

/*
 * Disk-specific ioctls.
 */
#define DIOCGETMEDIASIZE _IOR('d', 1, int)              /* get size in kbytes */
#define DIOCREINIT       _IO ('d', 2)                   /* re-initialize device */
#define DIOCGETPART      _IOR('d', 3, struct diskpart)  /* get partition */

#endif /* _SYS_DISK_H_ */
