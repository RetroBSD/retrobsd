/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
# include	"trek.h"
# include	<signal.h>
# include	<sys/wait.h>

/*
**  CALL THE SHELL
*/
void
shell()
{
	int		i;
	register int	pid;
	register sig_t	savint, savquit;

	pid = fork();
	if (! pid) {
		setuid(getuid());
		nice(0);
		execl("/bin/csh", "-", (char*)0);
		syserr("cannot execute /bin/csh");
	}
	savint = signal(SIGINT, SIG_IGN);
	savquit = signal(SIGQUIT, SIG_IGN);
	while (wait(&i) != pid)
                ;
	signal(SIGINT, savint);
	signal(SIGQUIT, savquit);
}
