/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifndef	_SYS_PROC_H_
#define	_SYS_PROC_H_

/*
 * One structure allocated per active
 * process. It contains all data needed
 * about the process while the
 * process may be swapped out.
 * Other per process data (user.h)
 * is swapped with the process.
 */
struct	proc {
	struct	proc *p_nxt;	/* linked list of allocated proc slots */
	struct	proc **p_prev;	/* also zombies, and free proc's */
	struct	proc *p_pptr;	/* pointer to process structure of parent */
	short	p_flag;
	short	p_uid;		/* user id, used to direct tty signals */
	short	p_pid;		/* unique process id */
	short	p_ppid;		/* process id of parent */
	long	p_sig;		/* signals pending to this process */
	int	p_stat;

	/*
	 * Union to overwrite information no longer needed by ZOMBIED
	 * process with exit information for the parent process.  The
	 * two structures have been carefully set up to use the same
	 * amount of memory.  Must be very careful that any values in
	 * p_alive are not used for zombies (zombproc).
	 */
	union {
	    struct {
		char	P_pri;		/* priority, negative is high */
		char	P_cpu;		/* cpu usage for scheduling */
		char	P_time;		/* resident time for scheduling */
		char	P_nice;		/* nice for cpu usage */
		char	P_slptime;	/* secs sleeping */
		char	P_ptracesig;	/* used between parent & traced child */
		struct proc *P_hash;	/* hashed based on p_pid */
		long	P_sigmask;	/* current signal mask */
		long	P_sigignore;	/* signals being ignored */
		long	P_sigcatch;	/* signals being caught by user */
		short	P_pgrp;		/* name of process group leader */
		struct	proc *P_link;	/* linked list of running processes */
		size_t	P_addr;		/* address of u. area */
		size_t	P_daddr;	/* address of data area */
		size_t	P_saddr;	/* address of stack area */
		size_t	P_dsize;	/* size of data area (clicks) */
		size_t	P_ssize;	/* size of stack segment (clicks) */
		caddr_t	P_wchan;	/* event process is awaiting */
		struct	k_itimerval P_realtimer;
	    } p_alive;
	    struct {
		int	P_xstat;	/* exit status for wait */
		struct k_rusage P_ru;	/* exit information */
	    } p_dead;
	} p_un;
};
#define	p_pri		p_un.p_alive.P_pri
#define	p_cpu		p_un.p_alive.P_cpu
#define	p_time		p_un.p_alive.P_time
#define	p_nice		p_un.p_alive.P_nice
#define	p_slptime	p_un.p_alive.P_slptime
#define	p_hash		p_un.p_alive.P_hash
#define	p_ptracesig	p_un.p_alive.P_ptracesig
#define	p_sigmask	p_un.p_alive.P_sigmask
#define	p_sigignore	p_un.p_alive.P_sigignore
#define	p_sigcatch	p_un.p_alive.P_sigcatch
#define	p_pgrp		p_un.p_alive.P_pgrp
#define	p_link		p_un.p_alive.P_link
#define	p_addr		p_un.p_alive.P_addr
#define	p_daddr		p_un.p_alive.P_daddr
#define	p_saddr		p_un.p_alive.P_saddr
#define	p_dsize		p_un.p_alive.P_dsize
#define	p_ssize		p_un.p_alive.P_ssize
#define	p_wchan		p_un.p_alive.P_wchan
#define	p_realtimer	p_un.p_alive.P_realtimer
#define	p_clktim	p_realtimer.it_value

#define	p_xstat		p_un.p_dead.P_xstat
#define	p_ru		p_un.p_dead.P_ru

#define	PIDHSZ		16
#define	PIDHASH(pid)	((pid) & (PIDHSZ - 1))

/* arguments to swapout: */
#define	X_OLDSIZE	(-1)	/* the old size is the same as current */
#define	X_DONTFREE	0	/* save core image (for parent in newproc) */
#define	X_FREECORE	1	/* free core space after swap */

#ifdef KERNEL
struct	proc *pidhash [PIDHSZ];
extern struct	proc proc[];	/* the proc table itself */
struct	proc *freeproc, *zombproc, *allproc, *qs;
			/* lists of procs in various states */
int nproc;

/*
 * Init the process queues.
 */
void pqinit (void);

/*
 * Find a process by pid.
 */
struct proc *pfind (int pid);

/*
 * Set user priority.
 */
int setpri (struct proc *pp);

/*
 * Send the specified signal to the specified process.
 */
void psignal (struct proc *p, int sig);

/*
 * Send the specified signal to a process group.
 */
void gsignal (int pgrp, int sig);

/*
 * Take the action for the specified signal.
 */
void postsig (int sig);

/*
 * If the current process has received a signal, return the signal number.
 */
int issignal (struct proc *p);

/*
 * Initialize signal state for process 0;
 * set to ignore signals that are ignored by default.
 */
void siginit (struct proc *p);

/*
 * Remove a process from its wait queue
 */
void unsleep (struct proc *p);

void selwakeup (struct proc *p, long coll);

/*
 * Set the process running;
 * arrange for it to be swapped in if necessary.
 */
void setrun (struct proc *p);

/*
 * Reschedule the CPU.
 */
void swtch (void);

/*
 * Recompute process priorities, once a second.
 */
void schedcpu (caddr_t arg);

/*
 * The main loop of the scheduling process. No return.
 */
void sched (void);

/*
 * Create a new process -- the internal version of system call fork.
 */
int newproc (int isvfork);

/*
 * Notify parent that vfork child is finished with parent's data.
 */
void endvfork (void);

/*
 * Put the process into the run queue.
 */
void setrq (struct proc *p);

/*
 * Remove runnable job from run queue.
 */
void remrq (struct proc *p);

/*
 * Exit the process.
 */
void exit (int rv);

/*
 * Swap I/O.
 */
void swap (size_t blkno, size_t coreaddr, int count, int rdflg);

/*
 * Kill a process when ran out of swap space.
 */
void swkill (struct proc *p, char *name);

/*
 * Give up the processor till a wakeup occurs on chan, at which time the
 * process enters the scheduling queue at priority pri.
 */
void sleep (caddr_t chan, int pri);

/*
 * Give up the processor till a wakeup occurs on ident or a timeout expires.
 * Then the process enters the scheduling queue at given priority.
 */
int tsleep (caddr_t ident, int priority, u_int timo);

/*
 * Arrange that given function is called in t/hz seconds.
 */
void timeout (void (*fun) (caddr_t), caddr_t arg, int t);

/*
 * Remove a function timeout call from the callout structure.
 */
void untimeout (void (*fun) (caddr_t), caddr_t arg);

/*
 * Handler for hardware clock interrupt.
 */
void hardclock (caddr_t pc, int ps);

/*
 * Swap out a process.
 */
void swapout (struct proc *p, int freecore, u_int odata, u_int ostack);

/*
 * Swap a process in.
 */
void swapin (struct proc *p);

/*
 * Is p an inferior of the current process?
 */
int inferior (struct proc *p);

/*
 * Test if the current user is the super user.
 */
int suser (void);

/*
 * Load from user area (probably swapped out): real uid,
 * controlling terminal device, and controlling terminal pointer.
 */
struct tty;
void fill_from_u (struct proc *p, uid_t *rup, struct tty **ttp, dev_t *tdp);

/*
 * Grow the stack to include the SP.
 */
int grow (unsigned sp);

/*
 * Kill current process with the specified signal in an uncatchable manner.
 */
void fatalsig (int signum);

/*
 * Parent controlled tracing.
 */
int procxmt (void);

#endif /* KERMEL */

/* stat codes */
#define	SSLEEP	1		/* awaiting an event */
#define	SWAIT	2		/* (abandoned state) */
#define	SRUN	3		/* running */
#define	SIDL	4		/* intermediate state in process creation */
#define	SZOMB	5		/* intermediate state in process termination */
#define	SSTOP	6		/* process being traced */

/* flag codes */
#define	SLOAD		0x0001	/* in core */
#define	SSYS		0x0002	/* swapper or pager process */
#define	SLOCK		0x0004	/* process being swapped out */
#define	SSWAP		0x0008	/* save area flag */
#define	P_TRACED	0x0010	/* process is being traced */
#define	P_WAITED	0x0020	/* another tracing flag */
#define	P_SINTR		0x0080	/* sleeping interruptibly */
#define	SVFORK		0x0100	/* process resulted from vfork() */
#define	SVFPRNT		0x0200	/* parent in vfork, waiting for child */
#define	SVFDONE		0x0400	/* parent has released child in vfork */
		     /* 0x0800	   unused */
#define	P_TIMEOUT	0x1000	/* tsleep timeout expired */
#define	P_NOCLDSTOP	0x2000	/* no SIGCHLD signal to parent */
#define	P_SELECT	0x4000	/* selecting; wakeup/waiting danger */
		     /* 0x8000	   unused */

#define	S_DATA	0		/* specified segment */
#define	S_STACK	1

#endif	/* !_SYS_PROC_H_ */
