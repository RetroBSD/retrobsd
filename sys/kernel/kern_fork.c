/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "systm.h"
#include "map.h"
#include "user.h"
#include "proc.h"
#include "inode.h"
#include "file.h"
#include "vm.h"
#include "kernel.h"
#include "syslog.h"

int	mpid;			/* generic for unique process id's */

/*
 * Create a new process -- the internal version of system call fork.
 * It returns 1 in the new process, 0 in the old.
 */
int
newproc (isvfork)
	int isvfork;
{
	register struct proc *child, *parent;
	register int n;
	static int pidchecked = 0;
	struct file *fp;

	/*
	 * First, just locate a slot for a process
	 * and copy the useful info from this process into it.
	 * The panic "cannot happen" because fork has already
	 * checked for the existence of a slot.
	 */
	mpid++;
retry:
	if (mpid >= 30000) {
		mpid = 100;
		pidchecked = 0;
	}
	if (mpid >= pidchecked) {
		int doingzomb = 0;

		pidchecked = 30000;
		/*
		 * Scan the proc table to check whether this pid
		 * is in use.  Remember the lowest pid that's greater
		 * than mpid, so we can avoid checking for a while.
		 */
		child = allproc;
again:
		for (; child != NULL; child = child->p_nxt) {
			if (child->p_pid == mpid || child->p_pgrp == mpid) {
				mpid++;
				if (mpid >= pidchecked)
					goto retry;
			}
			if (child->p_pid > mpid && pidchecked > child->p_pid)
				pidchecked = child->p_pid;
			if (child->p_pgrp > mpid && pidchecked > child->p_pgrp)
				pidchecked = child->p_pgrp;
		}
		if (!doingzomb) {
			doingzomb = 1;
			child = zombproc;
			goto again;
		}
	}
	child = freeproc;
	if (child == NULL)
		panic("no procs");

	freeproc = child->p_nxt;			/* off freeproc */

	/*
	 * Make a proc table entry for the new process.
	 */
	parent = u.u_procp;
	child->p_stat = SIDL;
	child->p_realtimer.it_value = 0;
	child->p_flag = SLOAD;
	child->p_uid = parent->p_uid;
	child->p_pgrp = parent->p_pgrp;
	child->p_nice = parent->p_nice;
	child->p_pid = mpid;
	child->p_ppid = parent->p_pid;
	child->p_pptr = parent;
	child->p_time = 0;
	child->p_cpu = 0;
	child->p_sigmask = parent->p_sigmask;
	child->p_sigcatch = parent->p_sigcatch;
	child->p_sigignore = parent->p_sigignore;
	/* take along any pending signals like stops? */
#ifdef UCB_METER
	if (isvfork) {
		forkstat.cntvfork++;
		forkstat.sizvfork += (parent->p_dsize + parent->p_ssize) >> 10;
	} else {
		forkstat.cntfork++;
		forkstat.sizfork += (parent->p_dsize + parent->p_ssize) >> 10;
	}
#endif
	child->p_wchan = 0;
	child->p_slptime = 0;
	{
	struct proc **hash = &pidhash [PIDHASH (child->p_pid)];

	child->p_hash = *hash;
	*hash = child;
	}
	/*
	 * some shuffling here -- in most UNIX kernels, the allproc assign
	 * is done after grabbing the struct off of the freeproc list.  We
	 * wait so that if the clock interrupts us and vmtotal walks allproc
	 * the text pointer isn't garbage.
	 */
	child->p_nxt = allproc;			/* onto allproc */
	child->p_nxt->p_prev = &child->p_nxt;	/*   (allproc is never NULL) */
	child->p_prev = &allproc;
	allproc = child;

	/*
	 * Increase reference counts on shared objects.
	 */
	for (n = 0; n <= u.u_lastfile; n++) {
		fp = u.u_ofile[n];
		if (fp == NULL)
			continue;
		fp->f_count++;
	}
	u.u_cdir->i_count++;
	if (u.u_rdir)
		u.u_rdir->i_count++;

	/*
	 * When the longjmp is executed for the new process,
	 * here's where it will resume.
	 */
	if (setjmp (&u.u_ssave)) {
		return(1);
	}

	child->p_dsize = parent->p_dsize;
	child->p_ssize = parent->p_ssize;
	child->p_daddr = parent->p_daddr;
	child->p_saddr = parent->p_saddr;

	/*
	 * Partially simulate the environment of the new process so that
	 * when it is actually created (by copying) it will look right.
	 */
	u.u_procp = child;

	/*
	 * Swap out the current process to generate the copy.
	 */
	parent->p_stat = SIDL;
	child->p_addr = parent->p_addr;
	child->p_stat = SRUN;
	swapout (child, X_DONTFREE, X_OLDSIZE, X_OLDSIZE);
	child->p_flag |= SSWAP;
	parent->p_stat = SRUN;
	u.u_procp = parent;

	if (isvfork) {
		/*
		 * Wait for the child to finish with it.
		 * RetroBSD: to make this work, significant
		 * changes in scheduler are required.
		 */
		parent->p_dsize = 0;
		parent->p_ssize = 0;
		child->p_flag |= SVFORK;
		parent->p_flag |= SVFPRNT;
		while (child->p_flag & SVFORK)
			sleep ((caddr_t)child, PSWP+1);
		if ((child->p_flag & SLOAD) == 0)
			panic ("newproc vfork");
		u.u_dsize = parent->p_dsize = child->p_dsize;
		parent->p_daddr = child->p_daddr;
		child->p_dsize = 0;
		u.u_ssize = parent->p_ssize = child->p_ssize;
		parent->p_saddr = child->p_saddr;
		child->p_ssize = 0;
		child->p_flag |= SVFDONE;
		wakeup ((caddr_t) parent);
		parent->p_flag &= ~SVFPRNT;
	}
	return(0);
}

static void
fork1 (isvfork)
	int isvfork;
{
	register int a;
	register struct proc *p1, *p2;

	a = 0;
	if (u.u_uid != 0) {
		for (p1 = allproc; p1; p1 = p1->p_nxt)
			if (p1->p_uid == u.u_uid)
				a++;
		for (p1 = zombproc; p1; p1 = p1->p_nxt)
			if (p1->p_uid == u.u_uid)
				a++;
	}
	/*
	 * Disallow if
	 *  No processes at all;
	 *  not su and too many procs owned; or
	 *  not su and would take last slot.
	 */
	p2 = freeproc;
	if (p2==NULL)
                log(LOG_ERR, "proc: table full\n");

	if (p2==NULL || (u.u_uid!=0 && (p2->p_nxt == NULL || a>MAXUPRC))) {
		u.u_error = EAGAIN;
		return;
	}
	p1 = u.u_procp;
	if (newproc (isvfork)) {
		/* Child */
		u.u_rval = 0;
		u.u_start = time.tv_sec;
		bzero(&u.u_ru, sizeof(u.u_ru));
		bzero(&u.u_cru, sizeof(u.u_cru));
		return;
	}
	/* Parent */
	u.u_rval = p2->p_pid;
}

/*
 * fork system call
 */
void
fork()
{
	fork1 (0);
}

/*
 * vfork system call, fast version of fork
 */
void
vfork()
{
	fork1 (1);
}
