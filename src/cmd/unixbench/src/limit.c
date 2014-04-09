/*******************************************************************************
 *  The BYTE UNIX Benchmarks - Release 3
 *          Module: limit.c   SID: 3.3 5/15/91 19:30:20
 *          
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Ben Smith, Rick Grehan or Tom Yager
 *	ben@bytepb.byte.com   rick_g@bytepb.byte.com   tyager@bytepb.byte.com
 *
 *******************************************************************************
 *  Modification Log:
 *  $Header: limit.c,v 3.4 87/06/22 14:25:11 kjmcdonell Beta $
 *
 ******************************************************************************/
/*
 *  Force a UNIX system to the per process and per user limits
 *
 */
char SCCSid[] = "@(#) @(#)limit.c:3.3 -- 5/15/91 19:30:20";

#define	CLICK	1024
#define	MAXCHN	100

#include <signal.h>
#include <setjmp.h>

int	parent;		/* parent's pid */
int	child;		/* child's pid */
int	pid[MAXCHN];
int	ncall;
int	level;
jmp_buf	env;

int main(argc, argv)
int	argc;
char	*argv[];
{
	char	*top;
	int	pad;
	int	end;
	int	i;
	int	status;
	float	f;
	int	flag();
	int	wakeup();
	long	last;

	/* open files (file descriptors) */
	for (i = 3; open(".", 0) > 0; i++) ;
	printf("Maximum open files per process: %d\n", i);
	while (--i > 2)
		close(i);

	/* process address space */
	top = (char *)sbrk(0);
#if debug
	printf("inital top of program: 0x%x\n", top);
#endif
	pad = (((int)top+CLICK-1)/CLICK)*CLICK - (int)top;
	sbrk(pad);
	for (i = 0; (char *)sbrk(CLICK) != (char *)-1; i++) ;
#if debug
	printf("final top of program: 0x%x\n", sbrk(0));
#endif
	brk(top);
#if debug
	printf("top of program restored to: 0x%x\n", sbrk(0));
#endif
	end = (((int)top+pad)/CLICK) + i;
	f = ((float)end * CLICK) / 1024;
	printf("Process address space limit: ");
	if (f < 1024)
		printf("%.2f Kbytes\n", f);
	else {
		f /= 1024;
		printf("%.2f Mbytes\n", f);
	}

	/* process creations */
	printf("Maximum number of child processes:");
	i = 0;
	while (1) {
#if debug
		printf("about to fork\n");
#endif
		if ((pid[i] = fork()) == -1) {
#if debug
			perror("fork failed");
#endif
			break;
		} else if (pid[i] != 0) {
#if debug
			printf("child %d: pid=%d\n", i+1, pid[i]);
#endif
			i++;
			if (i >= MAXCHN) {
				printf(" more than");
				break;
			}
		} else {
#if debug
			printf("child %d pausing\n", getpid());
#endif
			pause();
#if debug
			printf("child %d exiting\n", getpid());
#endif
			exit(1);
		}
	}
	printf(" %d\n", i);
	while (--i >= 0) {
		kill(pid[i], SIGKILL);
		wait(0);
	}

	ncall = level = 0;
	parent = getpid();
	signal(SIGTERM, flag);
	if ((child = fork()) == 0) {
		signal(SIGALRM, wakeup);
		recurse();
		exit(4);
	}
	while ((i = wait(&status)) == -1) {
	}
	printf("Estimated maximum stack size: %d Kbytes\n", level);
	exit(0);
}

recurse()
{
	int	temp[1024 / sizeof(int)];
#if debug
	printf("recursion @ level %d\n", ncall);
#endif
	temp[1024 / sizeof(int) - 1] = 1;
	ncall++;
	kill(parent, SIGTERM);
	while (ncall > level) {
		alarm(2);
		pause();
	}
	if (ncall < 8000)
		/* less than 8M bytes of temp storage! */
		recurse();
	else
		/* give up! */
		exit(0);
}

flag()
{
	signal(SIGTERM, flag);
	level++;
	if (child != 0)
		kill(child, SIGTERM);
}

wakeup()
{
	signal(SIGALRM, wakeup);
}
