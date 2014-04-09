/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)reboot.h	1.2 (2.11BSD GTE) 1996/5/9
 */

/*
 * Arguments to reboot system call.
 * These are passed to boot program in r4,
 * and on to init.
 */
#define	RB_AUTOBOOT	0	/* flags for system auto-booting itself */

#define	RB_ASKNAME	0x001	/* ask for file name to reboot from */
#define	RB_SINGLE	0x002	/* reboot to single user only */
#define	RB_NOSYNC	0x004	/* dont sync before reboot */
#define	RB_HALT		0x008	/* don't reboot, just halt */
#define	RB_INITNAME	0x010	/* name given for /etc/init */
#define	RB_DFLTROOT	0x020	/* use compiled-in rootdev */
#define	RB_DUMP		0x040	/* take a dump before rebooting */
#define	RB_NOFSCK	0x080	/* don't perform fsck's on reboot */
#define	RB_POWRFAIL	0x100	/* reboot caused by power failure */
#define	RB_RDONLY	0x200	/* mount root fs read-only */
#define	RB_AUTODEBUG	0x400	/* init runs autoconfig with "-d" (debug) */
#define	RB_POWEROFF	0x800	/* signal PSU to switch off power */
#define	RB_BOOTLOADER	0x1000	/* reboot into the bootloader */

#define	RB_PANIC	0	/* reboot due to panic */
#define	RB_BOOT		1	/* reboot due to boot() */
