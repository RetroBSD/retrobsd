/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#define	MSG_MAGIC	0x063061
#define	MSG_BSIZE	2048

struct	msgbuf {
	long	msg_magic;
	int	msg_bufx;
	int	msg_bufr;
	char	msg_bufc [MSG_BSIZE];
};

#define	logMSG	0		/* /dev/klog */

#ifdef KERNEL
/*
 * Check that log is open by a user program.
 */
int logisopen (int unit);
#endif
