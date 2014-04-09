/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Writearound to old stty system call.
 */
#include <sgtty.h>

int
stty(fd, ap)
	struct sgtty *ap;
{
	return ioctl (fd, TIOCSETP, ap);
}
