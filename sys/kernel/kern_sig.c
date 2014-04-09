/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "systm.h"
#include "user.h"
#include "inode.h"
#include "proc.h"
#include "namei.h"
#include "signalvar.h"

/*
 * Can the current process send the signal `signum' to process `q'?
 * This is complicated by the need to access the `real uid' of `q'.
 * The 'real uid' is in the u area and `q' may be (but usually is not) swapped
 * out.  Use the routine `fill_from_u' which the sysctl() call uses.  See the
 * notes in kern_sysctl.c
 *
 * The previous checks for a process to post a signal to another process
 * checked _only_ the effective userid.  With the implementation of the
 * 'saved id' feature and the ability of a setuid program to assume either
 * uid that check was inadequate.
 *
 * The 'c'urrent process is allowed to send a signal to a 't'arget process if
 * 1) either the real or effective user ids match OR 2) if the signal is
 * SIGCONT and the target process is a descendant of the current process
 */
static int
cansignal (q, signum)
	register struct proc *q;
	int	signum;
{
	register struct proc *curp = u.u_procp;
	uid_t	ruid;

	fill_from_u(q, &ruid, NULL, NULL);	/* XXX */
	if (curp->p_uid == 0 ||		/* c effective root */
	    u.u_ruid == ruid ||		/* c real = t real */
	    curp->p_uid == ruid ||		/* c effective = t real */
	    u.u_ruid == q->p_uid ||	/* c real = t effective */
	    curp->p_uid == q->p_uid ||	/* c effective = t effective */
	    (signum == SIGCONT && inferior(q)))
		return(1);
	return(0);
}

/*
 * 4.3 Compatibility
 */
void
sigstack()
{
	register struct a {
		struct	sigstack *nss;
		struct	sigstack *oss;
	} *uap = (struct a*) u.u_arg;
	struct sigstack ss;
	register int error = 0;

	ss.ss_sp = u.u_sigstk.ss_base;
	ss.ss_onstack = u.u_sigstk.ss_flags & SA_ONSTACK;
	if(uap->oss && (error = copyout ((caddr_t) &ss,
	    (caddr_t) uap->oss, sizeof (ss))))
		goto out;
	if (uap->nss && (error = copyin ((caddr_t) uap->nss,
	    (caddr_t) &ss, sizeof (ss))) == 0) {
		u.u_sigstk.ss_base = ss.ss_sp;
		u.u_sigstk.ss_size = 0;
		u.u_sigstk.ss_flags |= (ss.ss_onstack & SA_ONSTACK);
		u.u_psflags |= SAS_ALTSTACK;
	}
out:
	u.u_error = error;
}

static int
killpg1 (signo, pgrp, all)
	int signo, pgrp, all;
{
	register struct proc *p;
	int f, error = 0;

	if (! all && pgrp == 0) {
		/*
		 * Zero process id means send to my process group.
		 */
		pgrp = u.u_procp->p_pgrp;
		if (pgrp == 0)
			return (ESRCH);
	}
	for (f = 0, p = allproc; p != NULL; p = p->p_nxt) {
		if ((p->p_pgrp != pgrp && !all) || p->p_ppid == 0 ||
		    (p->p_flag&SSYS) || (all && p == u.u_procp))
			continue;
		if (! cansignal (p, signo)) {
			if (!all)
				error = EPERM;
			continue;
		}
		f++;
		if (signo)
			psignal(p, signo);
	}
	return (error ? error : (f == 0 ? ESRCH : 0));
}

void
kill()
{
	register struct a {
		int	pid;
		int	signo;
	} *uap = (struct a *)u.u_arg;
	register struct proc *p;
	register int error = 0;

	/*
	 * BSD4.3 botches the comparison against NSIG - it's a good thing for
	 * them psignal catches the error - however, since psignal is the
	 * kernel's internel signal mechanism and *should be getting correct
	 * parameters from the rest of the kernel, psignal shouldn't *have*
	 * to check it's parameters for validity.  If you feel differently,
	 * feel free to clutter up the entire inner kernel with parameter
	 * checks - start with postsig ...
	 */
	if (uap->signo < 0 || uap->signo >= NSIG) {
		error = EINVAL;
		goto out;
	}
	if (uap->pid > 0) {
		/* kill single process */
		p = pfind(uap->pid);
		if (p == 0) {
			error = ESRCH;
			goto out;
		}
		if (! cansignal (p, uap->signo))
			error = EPERM;
		else if (uap->signo)
			psignal (p, uap->signo);
		goto out;
	}
	switch (uap->pid) {
	case -1:		/* broadcast signal */
		error = killpg1 (uap->signo, 0, 1);
		break;
	case 0:			/* signal own process group */
		error = killpg1 (uap->signo, 0, 0);
		break;
	default:		/* negative explicit process group */
		error = killpg1 (uap->signo, -uap->pid, 0);
		break;
	}
out:
	u.u_error = error;
}

void
killpg()
{
	register struct a {
		int	pgrp;
		int	signo;
	} *uap = (struct a *)u.u_arg;
	register int error = 0;

	if (uap->signo < 0 || uap->signo >= NSIG) {
		error = EINVAL;
		goto out;
	}
	error = killpg1 (uap->signo, uap->pgrp, 0);
out:
	u.u_error = error;
}

/*
 * Put the argument process into the stopped
 * state and notify the parent via wakeup.
 * Signals are handled elsewhere.
 */
void
stop(p)
	register struct proc *p;
{
	p->p_stat = SSTOP;
	p->p_flag &= ~P_WAITED;
	wakeup((caddr_t)p->p_pptr);
}

/*
 * Send the specified signal to
 * all processes with 'pgrp' as
 * process group.
 */
void
gsignal (pgrp, sig)
	register int pgrp;
{
	register struct proc *p;

	if (pgrp == 0)
		return;

	for (p = allproc; p != NULL; p = p->p_nxt)
		if (p->p_pgrp == pgrp)
			psignal(p, sig);
}

/*
 * Send the specified signal to
 * the specified process.
 */
void
psignal(p, sig)
	register struct proc *p;
	register int sig;
{
	register int s;
	sig_t action;
	int prop;
	long mask;

	mask = sigmask(sig);
	prop = sigprop[sig];

	/*
	 * If proc is traced, always give parent a chance.
	 */
	if (p->p_flag & P_TRACED)
		action = SIG_DFL;
	else {
		/*
		 * If the signal is being ignored,
		 * then we forget about it immediately.
		 */
		if (p->p_sigignore & mask)
                        return;
		if (p->p_sigmask & mask)
			action = SIG_HOLD;
		else if (p->p_sigcatch & mask)
			action = SIG_CATCH;
		else
			action = SIG_DFL;
	}

	if (p->p_nice > NZERO && action == SIG_DFL && (prop & SA_KILL) &&
	    (p->p_flag & P_TRACED) == 0)
		p->p_nice = NZERO;

	if (prop & SA_CONT)
		p->p_sig &= ~stopsigmask;

	if (prop & SA_STOP) {
		/*
		 * If sending a tty stop signal to a member of an orphaned
		 * process group (i.e. a child of init), discard the signal
		 * here if the action is default; don't stop the process
		 * below if sleeping, and don't clear any pending SIGCONT.
		 */
		if ((prop & SA_TTYSTOP) && (p->p_pptr == &proc[1]) &&
		    action == SIG_DFL)
			return;
		p->p_sig &= ~contsigmask;
	}
	p->p_sig |= mask;

	/*
	 * Defer further processing for signals which are held.
	 */
	if (action == SIG_HOLD && ((prop & SA_CONT) == 0 || p->p_stat != SSTOP))
		return;
	s = splhigh();
	switch (p->p_stat) {

	case SSLEEP:
		/*
		 * If process is sleeping uninterruptibly we can not
		 * interrupt the sleep... the signal will be noticed
		 * when the process returns through trap() or syscall().
		 */
		if ((p->p_flag & P_SINTR) == 0)
			goto out;
		/*
		 * Process is sleeping and traced... make it runnable
		 * so it can discover the signal in issignal() and stop
		 * for the parent.
		 */
		if (p->p_flag & P_TRACED)
			goto run;

		/*
		 * If SIGCONT is default (or ignored) and process is
		 * asleep, we are finished; the process should not
		 * be awakened.
		 */
		if ((prop & SA_CONT) && action == SIG_DFL) {
			p->p_sig &= ~mask;
			goto out;
		}
		/*
		 * When a sleeping process receives a stop
		 * signal, process immediately if possible.
		 * All other (caught or default) signals
		 * cause the process to run.
		 */
		if (prop & SA_STOP) {
			if (action != SIG_DFL)
				goto run;
			/*
			 * If a child holding parent blocked,
			 * stopping could cause deadlock.
			 */
			if (p->p_flag & SVFORK)
				goto out;
			p->p_sig &= ~mask;
			p->p_ptracesig = sig;
			if ((p->p_pptr->p_flag & P_NOCLDSTOP) == 0)
				psignal(p->p_pptr, SIGCHLD);
			stop(p);
			goto out;
		} else
			goto run;
		/*NOTREACHED*/
	case SSTOP:
		/*
		 * If traced process is already stopped,
		 * then no further action is necessary.
		 */
		if (p->p_flag & P_TRACED)
			goto out;
		if (sig == SIGKILL)
			goto run;
		if (prop & SA_CONT) {
			/*
			 * If SIGCONT is default (or ignored), we continue the
			 * process but don't leave the signal in p_sig, as
			 * it has no further action.  If SIGCONT is held, we
			 * continue the process and leave the signal in
			 * p_sig.  If the process catches SIGCONT, let it
			 * handle the signal itself.  If it isn't waiting on
			 * an event, then it goes back to run state.
			 * Otherwise, process goes back to sleep state.
			 */
			if (action == SIG_DFL)
				p->p_sig &= ~mask;
			if (action == SIG_CATCH || p->p_wchan == 0)
				goto run;
			p->p_stat = SSLEEP;
			goto out;
		}

		if (prop & SA_STOP) {
			/*
			 * Already stopped, don't need to stop again.
			 * (If we did the shell could get confused.)
			 */
			p->p_sig &= ~mask;		/* take it away */
			goto out;
		}

		/*
		 * If process is sleeping interruptibly, then simulate a
		 * wakeup so that when it is continued, it will be made
		 * runnable and can look at the signal.  But don't make
		 * the process runnable, leave it stopped.
		 */
		if (p->p_wchan && (p->p_flag & P_SINTR))
			unsleep(p);
		goto out;
		/*NOTREACHED*/

	default:
		/*
		 * SRUN, SIDL, SZOMB do nothing with the signal,
		 * other than kicking ourselves if we are running.
		 * It will either never be noticed, or noticed very soon.
		 */
		goto out;
	}
	/*NOTREACHED*/
run:
	/*
	 * Raise priority to at least PUSER.
	 */
	if (p->p_pri > PUSER)
		p->p_pri = PUSER;
	setrun(p);
out:
	splx(s);
}

/*
 * If the current process has received a signal (should be caught
 * or cause termination, should interrupt current syscall) return the
 * signal number.  Stop signals with default action are processed
 * immediately then cleared; they are not returned.  This is checked
 * after each entry into the kernel for a syscall of trap (though this
 * can usually be done without calling issignal by checking the pending
 * signals masks in CURSIG)/  The normal sequence is:
 *
 *	while (signum = CURSIG(u.u_procp))
 *		postsig(signum);
 */
int
issignal (p)
	register struct proc *p;
{
	register int sig;
	long mask;
	int prop;

	for (;;) {
		mask = p->p_sig & ~p->p_sigmask;
		if (p->p_flag&SVFORK)
			mask &= ~stopsigmask;
		if (mask == 0)
			return(0);		/* No signals to send */
		sig = ffs(mask);
		mask = sigmask(sig);
		prop = sigprop[sig];
		/*
		 * We should see pending but ignored signals
		 * only if P_TRACED was on when they were posted.
		*/
		if ((mask & p->p_sigignore) && ! (p->p_flag & P_TRACED)) {
			p->p_sig &= ~mask;
			continue;
		}
		if ((p->p_flag & P_TRACED) && ! (p->p_flag & SVFORK)) {
			/*
			 * If traced, always stop, and stay
			 * stopped until released by the parent.
			 *
			 * Note that we must clear the pending signal
			 * before we call procxmt since that routine
			 * might cause a fault, calling sleep and
			 * leading us back here again with the same signal.
			 * Then we would be deadlocked because the tracer
			 * would still be blocked on the ipc struct from
			 * the initial request.
			 */
			p->p_sig &= ~mask;
			p->p_ptracesig = sig;
			psignal(p->p_pptr, SIGCHLD);
			do {
				stop(p);
				swtch();
			} while (! procxmt() && (p->p_flag & P_TRACED));

			/*
			 * If parent wants us to take the signal,
			 * then it will leave it in p->p_ptracesig;
			 * otherwise we just look for signals again.
			 */
			sig = p->p_ptracesig;
			if (sig == 0)
				continue;

			/*
			 * Put the new signal into p_sig.  If the
			 * signal is being masked, look for other signals.
			 */
			mask = sigmask(sig);
			p->p_sig |= mask;
			if (p->p_sigmask & mask)
				continue;

			/*
			 * If the traced bit got turned off, go back up
			 * to the top to rescan signals.  This ensures
			 * that p_sig* and u_signal are consistent.
			 */
			if ((p->p_flag& P_TRACED) == 0)
				continue;
			prop = sigprop[sig];
		}

		switch ((int)u.u_signal[sig]) {

		case (int)SIG_DFL:
			/*
			 * Don't take default actions on system processes.
			 */
			if (p->p_pid <= 1) {
#ifdef DIAGNOSTIC
				/*
 				 * Are you sure you want to ignore SIGSEGV
 				 * in init? XXX
				 */
				printf("Process (pid %d) got signal %d\n",
					p->p_pid, sig);
#endif
				break;
			}
			/*
			 * If there is a pending stop signal to process
			 * with default action, stop here,
			 * then clear the signal.  However,
			 * if process is member of an orphaned
			 * process group, ignore tty stop signals.
			 */
			if (prop & SA_STOP) {
				if (p->p_flag & P_TRACED ||
		    		    (p->p_pptr == &proc[1] &&
				    prop & SA_TTYSTOP))
					break;	/* == ignore */
				p->p_ptracesig = sig;
				if ((p->p_pptr->p_flag & P_NOCLDSTOP) == 0)
					psignal(p->p_pptr, SIGCHLD);
				stop(p);
				swtch();
				break;
			} else if (prop & SA_IGNORE) {
				/*
				 * Except for SIGCONT, shouldn't get here.
				 * Default action is to ignore; drop it.
				 */
				break;		/* == ignore */
			} else
				return(sig);
			/*NOTREACHED*/

		case (int)SIG_IGN:
			/*
			 * Masking above should prevent us
			 * ever trying to take action on a held
			 * or ignored signal, unless process is traced.
			 */
			if ((prop & SA_CONT) == 0 &&
				(p->p_flag & P_TRACED) == 0)
				printf("issig\n");
			break;			/* == ignore */

		default:
			/*
			 * This signal has an action, let postsig process it.
			 */
			return(sig);
		}
		p->p_sig &= ~mask;		/* take the signal away! */
	}
	/* NOTREACHED */
}

/*
 * Create a core image on the file "core"
 * If you are looking for protection glitches,
 * there are probably a wealth of them here
 * when this occurs to a suid command.
 *
 * It writes UPAGES (USIZE for pdp11) block of the
 * user.h area followed by the entire
 * data+stack segments.
 */
static int
core()
{
	register struct inode *ip;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;
	register char *np;
	char	*cp, name[MAXCOMLEN + 6];

	/*
	 * Don't dump if not root.
	 */
	if (! suser())
		return(0);
	if (USIZE + u.u_dsize + u.u_ssize >= u.u_rlimit[RLIMIT_CORE].rlim_cur)
		return (0);
	cp = u.u_comm;
	np = name;
	while ((*np++ = *cp++))
		;
	cp = ".core";
	np--;
	while ((*np++ = *cp++))
		;
	u.u_error = 0;
	NDINIT (ndp, CREATE, FOLLOW, name);
	ip = namei(ndp);
	if (ip == NULL) {
		if (u.u_error)
			return (0);
		ip = maknode (0644, ndp);
		if (ip==NULL)
			return (0);
	}
	if (access(ip, IWRITE) ||
	   (ip->i_mode&IFMT) != IFREG ||
	   ip->i_nlink != 1) {
		u.u_error = EFAULT;
		goto out;
	}
	itrunc(ip, (u_long)0, 0);
	u.u_error = rdwri (UIO_WRITE, ip, (caddr_t) &u,
                USIZE, (off_t) 0, IO_UNIT, (int*) 0);
	if (u.u_error)
		goto out;

	u.u_error = rdwri (UIO_WRITE, ip, (caddr_t) USER_DATA_START,
                u.u_dsize, (off_t) USIZE, IO_UNIT, (int*) 0);
	if (u.u_error)
		goto out;

	u.u_error = rdwri (UIO_WRITE, ip, (caddr_t) USER_DATA_END - u.u_ssize,
                u.u_ssize, (off_t) USIZE + u.u_dsize,
		IO_UNIT, (int*) 0);
out:
	iput(ip);
	return (u.u_error == 0);
}

/*
 * Take the action for the specified signal
 * from the current set of pending signals.
 */
void
postsig(sig)
	int sig;
{
	register struct proc *p = u.u_procp;
	long mask = sigmask(sig), returnmask;
	register sig_t action;

	p->p_sig &= ~mask;
	action = u.u_signal[sig];

	if (action != SIG_DFL) {
#ifdef DIAGNOSTIC
		if (action == SIG_IGN || (p->p_sigmask & mask))
			panic("postsig action");
#endif
		u.u_error = 0;	/* XXX - why? */
		/*
		 * Set the new mask value and also defer further
		 * occurences of this signal.
		 *
		 * Special case: user has done a sigsuspend.  Here the
		 * current mask is not of interest, but rather the
		 * mask from before the sigsuspend is what we want restored
		 * after the signal processing is completed.
		 */
		(void) splhigh();
		if (u.u_psflags & SAS_OLDMASK) {
			returnmask = u.u_oldmask;
			u.u_psflags &= ~SAS_OLDMASK;
		} else
			returnmask = p->p_sigmask;
		p->p_sigmask |= u.u_sigmask[sig] | mask;
		(void) spl0();
		u.u_ru.ru_nsignals++;
		sendsig(action, sig, returnmask);
		return;
	}
	if (sigprop[sig] & SA_CORE) {
		u.u_arg[0] = sig;
		if (core())
			sig |= 0200;
	}
	exit(sig);
}
