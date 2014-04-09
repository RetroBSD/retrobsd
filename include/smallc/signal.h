/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifndef	NSIG

#define NSIG	32

#define	SIGHUP	1	/* hangup */
#define	SIGINT	2	/* interrupt */
#define	SIGQUIT	3	/* quit */
#define	SIGILL	4	/* illegal instruction (not reset when caught) */
#define	SIGTRAP	5	/* trace trap (not reset when caught) */
#define	SIGIOT	6	/* IOT instruction */
#define	SIGABRT	SIGIOT	/* compatibility */
#define	SIGEMT	7	/* EMT instruction */
#define	SIGFPE	8	/* floating point exception */
#define	SIGKILL	9	/* kill (cannot be caught or ignored) */
#define	SIGBUS	10	/* bus error */
#define	SIGSEGV	11	/* segmentation violation */
#define	SIGSYS	12	/* bad argument to system call */
#define	SIGPIPE	13	/* write on a pipe with no one to read it */
#define	SIGALRM	14	/* alarm clock */
#define	SIGTERM	15	/* software termination signal from kill */
#define	SIGURG	16	/* urgent condition on IO channel */
#define	SIGSTOP	17	/* sendable stop signal not from tty */
#define	SIGTSTP	18	/* stop signal from tty */
#define	SIGCONT	19	/* continue a stopped process */
#define	SIGCHLD	20	/* to parent on child stop or exit */
#define	SIGCLD	SIGCHLD	/* compatibility */
#define	SIGTTIN	21	/* to readers pgrp upon background tty read */
#define	SIGTTOU	22	/* like TTIN for output if (tp->t_local&LTOSTOP) */
#define	SIGIO	23	/* input/output possible signal */
#define	SIGXCPU	24	/* exceeded CPU time limit */
#define	SIGXFSZ	25	/* exceeded file size limit */
#define	SIGVTALRM 26	/* virtual time alarm */
#define	SIGPROF	27	/* profiling time alarm */
#define SIGWINCH 28	/* window size changes */
#define SIGUSR1 30	/* user defined signal 1 */
#define SIGUSR2 31	/* user defined signal 2 */

#define	SIG_ERR		-1
#define	SIG_DFL		0
#define	SIG_IGN		1
#define	BADSIG          SIG_ERR

#define SA_ONSTACK	0x0001	/* take signal on signal stack */
#define SA_RESTART	0x0002	/* restart system on signal return */
#define	SA_DISABLE	0x0004	/* disable taking signals on alternate stack */
#define SA_NOCLDSTOP	0x0008	/* do not generate SIGCHLD on child stop */

/*
 * Flags for sigprocmask:
 */
#define	SIG_BLOCK	1	/* block specified signal set */
#define	SIG_UNBLOCK	2	/* unblock specified signal set */
#define	SIG_SETMASK	3	/* set specified signal set */

#define	MINSIGSTKSZ	128			/* minimum allowable stack */
#define	SIGSTKSZ	(MINSIGSTKSZ + 384)	/* recommended stack size */

#define SV_ONSTACK	SA_ONSTACK	/* take signal on signal stack */
#define SV_INTERRUPT	SA_RESTART	/* same bit, opposite sense */

/*
 * Macro for converting signal number to a mask suitable for
 * sigblock().
 */
#define sigmask(m)		(1L << ((m)-1))
#define sigaddset(set, signo)	(*(set) |= 1L << ((signo) - 1), 0)
#define sigdelset(set, signo)	(*(set) &= ~(1L << ((signo) - 1)), 0)
#define sigemptyset(set)	(*(set) = (sigset_t)0, (int)0)
#define sigfillset(set)         (*(set) = ~(sigset_t)0, (int)0)
#define sigismember(set, signo) ((*(set) & (1L << ((signo) - 1))) != 0)

#endif /* NSIG */
