/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef	_SYS_DISK_H_
#define	_SYS_DISK_H_
#include <sys/disklabel.h>

/*
 * Disk device structures.
 *
 * Note that this is only a preliminary outline.  The final disk structures
 * may be somewhat different.
 *
 * Note:  the 2.11BSD version is very different.  The 4.4 version served
 *        as the inspiration. I needed something similar but for slightly
 *	  different purposes.
 */

/*
 * Disk device structures.  Rather than replicate driver specific variations
 * of the following in each driver it was made common here.
 *
 * Some of the flags are specific to various drivers. For example ALIVE and
 * ONLINE apply to MSCP devices more than to SMD devices while the SEEK flag
 * applies to the SMD (xp) driver but not to the MSCP driver.  The rest
 * of the flags as well as the open partition bitmaps are usable by any disk
 * driver.  One 'dkdevice' structure is needed for each disk drive supported
 * by a driver.
 *
 * The entire disklabel is not resident in the kernel address space.  Only
 * the partition table is directly accessible by the kernel.  The MSCP driver
 * does not care (or know) about the geometry of the disk.  Not holding
 * the entire label in the kernel saved quite a bit of D space.  Other drivers
 * which need geometry information from the label will have to map in the
 * label and copy out the geometry data they require.  This is unlikely to
 * cause much overhead since labels are read and written infrequently - when
 * mounting a drive, assigning a label, running newfs, etc.
 */

struct dkdevice {
	int	dk_bopenmask;		/* block devices open */
	int	dk_copenmask;		/* character devices open */
	int	dk_openmask;		/* composite (bopen|copen) */
	int	dk_flags;		/* label state   see below */
	size_t	dk_label;		/* sector containing label */
	struct	partition dk_parts[MAXPARTITIONS];	/* inkernel portion */
};

#define	DKF_OPENING	0x0001		/* drive is being opened */
#define	DKF_CLOSING	0x0002		/* drive is being closed */
#define	DKF_WANTED	0x0004		/* drive is being waited for */
#define	DKF_ALIVE	0x0008		/* drive is alive */
#define	DKF_ONLINE	0x0010		/* drive is online */
#define	DKF_WLABEL	0x0020		/* label area is being written */
#define	DKF_SEEK	0x0040		/* drive is seeking */
#define	DKF_SWAIT	0x0080		/* waiting for seek to complete */

/* encoding of disk minor numbers, should be elsewhere... but better
 * here than in ufs_disksubr.c
 *
 * Note: the controller number in bits 6 and 7 of the minor device are NOT
 *	 removed.  It is the responsibility of the driver to extract or mask
 *	 these bits.
*/

#define dkunit(dev)		(minor(dev) >> 3)
#define dkpart(dev)		(minor(dev) & 07)
#define dkminor(unit, part)	(((unit) << 3) | (part))

#ifdef KERNEL
//char	*readdisklabel();
//int	setdisklabel();
//int	writedisklabel();
#endif
#endif /* _SYS_DISK_H_ */
