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
#include "vm.h"
#include "file.h"
#include "wait.h"
#include "kernel.h"

/*
 * Notify parent that vfork child is finished with parent's data.  Called
 * during exit/exec(getxfile).  The child
 * must be locked in core so it will be in core when the parent runs.
 */
void
endvfork()
{
	register struct proc *rip, *rpp;

	rpp = u.u_procp;
	rip = rpp->p_pptr;
	rpp->p_flag &= ~SVFORK;
	rpp->p_flag |= SLOCK;
	wakeup ((caddr_t) rpp);
	while (! (rpp->p_flag & SVFDONE))
		sleep ((caddr_t) rip, PZERO-1);
	/*
	 * The parent has taken back our data+stack, set our sizes to 0.
	 */
	u.u_dsize = rpp->p_dsize = 0;
	u.u_ssize = rpp->p_ssize = 0;
	rpp->p_flag &= ~(SVFDONE | SLOCK);
}

/*
 * Exit: deallocate address space and other resources,
 * change proc state to zombie, and unlink proc from allproc
 * list.  Save exit status and rusage for wait4().
 * Check for child processes and orphan them.
 */
void
exit (rv)
	int rv;
{
	register int i;
	register struct proc *p;
	struct	proc **pp;

	p = u.u_procp;
	p->p_flag &= ~P_TRACED;
	p->p_sigignore = ~0;
	p->p_sig = 0;
	/*
	 * 2.11 doesn't need to do this and it gets overwritten anyway.
	 * p->p_realtimer.it_value = 0;
	 */
	for (i = 0; i <= u.u_lastfile; i++) {
		register struct file *f;

		f = u.u_ofile[i];
		u.u_ofile[i] = NULL;
		u.u_pofile[i] = 0;
		(void) closef(f);
	}
	ilock(u.u_cdir);
	iput(u.u_cdir);
	if (u.u_rdir) {
		ilock(u.u_rdir);
		iput(u.u_rdir);
	}
	u.u_rlimit[RLIMIT_FSIZE].rlim_cur = RLIM_INFINITY;

	if (p->p_flag & SVFORK)
		endvfork();

	if (p->p_pid == 1)
		panic("init died");
	if ((*p->p_prev = p->p_nxt) != NULL)		/* off allproc queue */
		p->p_nxt->p_prev = p->p_prev;
	p->p_nxt = zombproc;                            /* onto zombproc */
	if (p->p_nxt != NULL)
		p->p_nxt->p_prev = &p->p_nxt;
	p->p_prev = &zombproc;
	zombproc = p;
	p->p_stat = SZOMB;

	noproc = 1;
	for (pp = &pidhash[PIDHASH(p->p_pid)]; *pp; pp = &(*pp)->p_hash)
		if (*pp == p) {
			*pp = p->p_hash;
			goto done;
		}
	panic("exit");
done:
	/*
	 * Overwrite p_alive substructure of proc - better not be anything
	 * important left!
	 */
	p->p_xstat = rv;
	p->p_ru = u.u_ru;
	ruadd(&p->p_ru, &u.u_cru);
	{
		register struct proc *q;
		int doingzomb = 0;

		q = allproc;
again:
		for(; q; q = q->p_nxt)
			if (q->p_pptr == p) {
				q->p_pptr = &proc[1];
				q->p_ppid = 1;
				wakeup((caddr_t)&proc[1]);
				if (q->p_flag& P_TRACED) {
					q->p_flag &= ~P_TRACED;
					psignal(q, SIGKILL);
				} else if (q->p_stat == SSTOP) {
					psignal(q, SIGHUP);
					psignal(q, SIGCONT);
				}
			}
		if (!doingzomb) {
			doingzomb = 1;
			q = zombproc;
			goto again;
		}
	}
	psignal(p->p_pptr, SIGCHLD);
	wakeup((caddr_t) p->p_pptr);
        wakeup((caddr_t) &runin);
	swtch();
	/* NOTREACHED */
}

/*
 * exit system call: pass back caller's arg
 */
void
rexit()
{
	register struct a {
		int	rval;
	} *uap = (struct a*) u.u_arg;

	exit (W_EXITCODE (uap->rval, 0));
	/* NOTREACHED */
}

struct args {
	int pid;
	int *status;
	int options;
	struct rusage *rusage;
};

/*
 * Wait: check child processes to see if any have exited,
 * stopped under trace or (optionally) stopped by a signal.
 * Pass back status and make available for reuse the exited
 * child's proc structure.
 */
static int
wait1 (q, uap, retval)
	struct proc *q;
	register struct args *uap;
	int retval[];
{
	int nfound, status;
	struct rusage ru;			/* used for local conversion */
	register struct proc *p;
	register int error;

	if (uap->pid == WAIT_MYPGRP)		/* == 0 */
		uap->pid = -q->p_pgrp;
loop:
	nfound = 0;
	/*
	 * 4.X has child links in the proc structure, so they consolidate
	 * these two tests into one loop.  We only have the zombie chain
	 * and the allproc chain, so we check for ZOMBIES first, then for
	 * children that have changed state.  We check for ZOMBIES first
	 * because they are more common, and, as the list is typically small,
	 * a faster check.
	 */
	for (p = zombproc; p; p = p->p_nxt) {
		if (p->p_pptr != q)	/* are we the parent of this process? */
			continue;
		if (uap->pid != WAIT_ANY &&
		    p->p_pid != uap->pid && p->p_pgrp != -uap->pid)
			continue;
		retval[0] = p->p_pid;
		retval[1] = p->p_xstat;
		if (uap->status && (error = copyout ((caddr_t) &p->p_xstat,
		    (caddr_t) uap->status, sizeof (uap->status))))
			return(error);
		if (uap->rusage) {
			rucvt(&ru, &p->p_ru);
			error = copyout ((caddr_t) &ru, (caddr_t) uap->rusage, sizeof (ru));
			if (error)
				return(error);
		}
		ruadd(&u.u_cru, &p->p_ru);
		p->p_xstat = 0;
		p->p_stat = NULL;
		p->p_pid = 0;
		p->p_ppid = 0;
		if ((*p->p_prev = p->p_nxt) != NULL)	/* off zombproc */
			p->p_nxt->p_prev = p->p_prev;
		p->p_nxt = freeproc;			/* onto freeproc */
		freeproc = p;
		p->p_pptr = 0;
		p->p_sig = 0;
		p->p_sigcatch = 0;
		p->p_sigignore = 0;
		p->p_sigmask = 0;
		p->p_pgrp = 0;
		p->p_flag = 0;
		p->p_wchan = 0;
		return (0);
	}
	for (p = allproc; p;p = p->p_nxt) {
		if (p->p_pptr != q)
			continue;
		if (uap->pid != WAIT_ANY &&
		    p->p_pid != uap->pid && p->p_pgrp != -uap->pid)
			continue;
		++nfound;
		if (p->p_stat == SSTOP && ! (p->p_flag & P_WAITED) &&
		    (p->p_flag & P_TRACED || uap->options & WUNTRACED)) {
			p->p_flag |= P_WAITED;
			retval[0] = p->p_pid;
			error = 0;
			if (uap->status) {
				status = W_STOPCODE(p->p_ptracesig);
				error = copyout ((caddr_t) &status,
					(caddr_t) uap->status, sizeof (status));
			}
			return (error);
		}
	}
	if (nfound == 0)
		return (ECHILD);
	if (uap->options&WNOHANG) {
		retval[0] = 0;
		return (0);
	}
	error = tsleep ((caddr_t) q, PWAIT|PCATCH, 0);
	if (error == 0)
		goto loop;
	return(error);
}

void
wait4()
{
	int retval[2];
	register struct	args *uap = (struct args*) u.u_arg;

	retval[0] = 0;
	u.u_error = wait1 (u.u_procp, uap, retval);
	if (! u.u_error)
		u.u_rval = retval[0];
}

void
reboot()
{
	struct a {
		int	opt;
	};

	if (suser ())
		boot (rootdev, ((struct a*)u.u_arg)->opt);
}
