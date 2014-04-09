/* Copyright 1988,1990,1993,1994 by Paul Vixie
 * All rights reserved
 *
 * Distribute freely, except: don't remove my name from the source or
 * documentation (don't take credit for my work), mark your changes (don't
 * get me blamed for your possible bugs), don't alter or remove this
 * notice.  May be sold if buildable source is provided to buyer.  No
 * warrantee of any kind, express or implied, is included with this
 * software; use at your own risk, responsibility for damages (if any) to
 * anyone resulting from the use of this software rests entirely with the
 * user.
 *
 * Send bug reports, bug fixes, enhancements, requests, flames, etc., and
 * I'll try to keep a version up to date.  I can be reached as follows:
 * Paul Vixie          <paul@vix.com>          uunet!decwrl!vixie!paul
 */

#if !defined(lint) && !defined(LINT)
static char rcsid[] = "$Id: compat.c,v 1.6 1994/01/15 20:43:43 vixie Exp $";
#endif

/* vix 30dec93 [broke this out of misc.c - see RCS log for history]
 * vix 15jan87 [added TIOCNOTTY, thanks csg@pyramid]
 */


#include "cron.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>

/*
 * Ripped off from daemon(3) - differences are this sets the process group
 * and does not fork (because that has been done already).
*/
int
setsid()
{
	int	newpgrp;
	register int	fd;

	newpgrp = setpgrp(0, getpid());
	if	((fd = open(_PATH_TTY, 2)) >= 0)
		{
		(void) ioctl(fd, TIOCNOTTY, (char*)0);
		(void) close(fd);
		}
	if	((fd = open(_PATH_DEVNULL, O_RDWR, 0)) != -1)
		{
		(void)dup2(fd, 0);
		(void)dup2(fd, 1);
		(void)dup2(fd, 2);
		if	(fd > 2)
			(void)close(fd);
		}
	return newpgrp;
}
