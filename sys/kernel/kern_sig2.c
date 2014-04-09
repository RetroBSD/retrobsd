/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * This module is a hacked down version of kern_sig.c from 4.4BSD.  The
 * original signal handling code is still present in 2.11's kern_sig.c.  This
 * was done because large modules are very hard to fit into the kernel's
 * overlay structure.  A smaller kern_sig2.c fits more easily into an overlaid
 * kernel.
 */
#define	SIGPROP		/* include signal properties table */
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/dir.h>
#include <sys/namei.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/user.h>		/* for coredump */

static void
setsigvec(signum, sa)
	int signum;
	register struct sigaction *sa;
{
	unsigned long bit;
	register struct proc *p = u.u_procp;

	bit = sigmask(signum);
	/*
	 * Change setting atomically.
	 */
	(void) splhigh();
	u.u_signal[signum] = sa->sa_handler;
	u.u_sigmask[signum] = sa->sa_mask &~ sigcantmask;
	if ((sa->sa_flags & SA_RESTART) == 0)
		u.u_sigintr |= bit;
	else
		u.u_sigintr &= ~bit;
	if (sa->sa_flags & SA_ONSTACK)
		u.u_sigonstack |= bit;
	else
		u.u_sigonstack &= ~bit;
	if (signum == SIGCHLD) {
		if (sa->sa_flags & SA_NOCLDSTOP)
			p->p_flag |= P_NOCLDSTOP;
		else
			p->p_flag &= ~P_NOCLDSTOP;
	}
	/*
	 * Set bit in p_sigignore for signals that are set to SIG_IGN,
	 * and for signals set to SIG_DFL where the default is to ignore.
	 * However, don't put SIGCONT in p_sigignore,
	 * as we have to restart the process.
	 */
	if (sa->sa_handler == SIG_IGN ||
	    (sigprop[signum] & SA_IGNORE && sa->sa_handler == SIG_DFL)) {
		p->p_sig &= ~bit;		/* never to be seen again */
		if (signum != SIGCONT)
			p->p_sigignore |= bit;	/* easier in psignal */
		p->p_sigcatch &= ~bit;
	} else {
		p->p_sigignore &= ~bit;
		if (sa->sa_handler == SIG_DFL)
			p->p_sigcatch &= ~bit;
		else
			p->p_sigcatch |= bit;
	}
	(void) spl0();
}

void
sigaction()
{
	register struct a {
		int	signum;
		struct	sigaction *nsa;
		struct	sigaction *osa;
		u_int	sigtramp;
	} *uap = (struct a *)u.u_arg;
	struct sigaction vec;
	register struct sigaction *sa;
	register int signum;
	u_long bit;
	int error = 0;

	u.u_sigtramp = uap->sigtramp;	/* save trampoline address */

	signum = uap->signum;
	if (signum <= 0 || signum >= NSIG) {
		error = EINVAL;
		goto out;
	}
	if (uap->nsa && (signum == SIGKILL || signum == SIGSTOP)) {
		error = EINVAL;
		goto out;
	}
	sa = &vec;
	if (uap->osa) {
		sa->sa_handler = u.u_signal[signum];
		sa->sa_mask = u.u_sigmask[signum];
		bit = sigmask(signum);
		sa->sa_flags = 0;
		if ((u.u_sigonstack & bit) != 0)
			sa->sa_flags |= SA_ONSTACK;
		if ((u.u_sigintr & bit) == 0)
			sa->sa_flags |= SA_RESTART;
		if (u.u_procp->p_flag & P_NOCLDSTOP)
			sa->sa_flags |= SA_NOCLDSTOP;
		error = copyout ((caddr_t) sa, (caddr_t) uap->osa, sizeof(vec));
		if (error != 0)
			goto out;
	}
	if (uap->nsa) {
		error = copyin ((caddr_t) uap->nsa, (caddr_t) sa, sizeof(vec));
		if (error != 0)
			goto out;
		setsigvec(signum, sa);
	}
out:
	u.u_error = error;
}

/*
 * Kill current process with the specified signal in an uncatchable manner;
 * used when process is too confused to continue, or we are unable to
 * reconstruct the process state safely.
 */
void
fatalsig(signum)
	int signum;
{
	unsigned long mask;
	register struct proc *p = u.u_procp;

	u.u_signal[signum] = SIG_DFL;
	mask = sigmask(signum);
	p->p_sigignore &= ~mask;
	p->p_sigcatch &= ~mask;
	p->p_sigmask &= ~mask;
	psignal(p, signum);
}

/*
 * Initialize signal state for process 0;
 * set to ignore signals that are ignored by default.
 */
void
siginit(p)
	register struct proc *p;
{
	register int i;

	for (i = 0; i < NSIG; i++)
		if (sigprop[i] & SA_IGNORE && i != SIGCONT)
			p->p_sigignore |= sigmask(i);
}

/*
 * Manipulate signal mask.
 * Unlike 4.4BSD we do not receive a pointer to the new and old mask areas and
 * do a copyin/copyout instead of storing indirectly thru a 'retval' parameter.
 * This is because we have to return both an error indication (which is 16 bits)
 * _AND_ the new mask (which is 32 bits).  Can't do both at the same time with
 * the 2BSD syscall return mechanism.
 */
void
sigprocmask()
{
	register struct a {
		int how;
		sigset_t *set;
		sigset_t *oset;
	} *uap = (struct a *)u.u_arg;
	int error = 0;
	sigset_t oldmask, newmask;
	register struct proc *p = u.u_procp;

	oldmask = p->p_sigmask;
	if (! uap->set)	/* No new mask, go possibly return old mask */
		goto out;
	error = copyin ((caddr_t) uap->set, (caddr_t) &newmask, sizeof (newmask));
	if (error)
		goto out;
	(void) splhigh();

	switch (uap->how) {
	case SIG_BLOCK:
		p->p_sigmask |= (newmask &~ sigcantmask);
		break;
	case SIG_UNBLOCK:
		p->p_sigmask &= ~newmask;
		break;
	case SIG_SETMASK:
		p->p_sigmask = newmask &~ sigcantmask;
		break;
	default:
		error = EINVAL;
		break;
	}
	(void) spl0();
out:
	if (error == 0 && uap->oset)
		error = copyout ((caddr_t) &oldmask, (caddr_t) uap->oset, sizeof (oldmask));
	u.u_error = error;
}

/*
 * sigpending and sigsuspend use the standard calling sequence unlike 4.4 which
 * used a nonstandard (mask instead of pointer) calling convention.
 */
void
sigpending()
{
	register struct a {
		struct sigset_t *set;
	} *uap = (struct a *)u.u_arg;
	register int error = 0;
	struct	proc *p = u.u_procp;

	if (uap->set)
		error = copyout((caddr_t)&p->p_sig, (caddr_t)uap->set,
				sizeof (p->p_sig));
	else
		error = EINVAL;
	u.u_error = error;
}

/*
 * sigsuspend is supposed to always return EINTR so we ignore errors on the
 * copyin by assuming a mask of 0.
 */
void
sigsuspend()
{
	register struct a {
		struct sigset_t *set;
	} *uap = (struct a *)u.u_arg;
	sigset_t nmask = 0;
	struct proc *p = u.u_procp;
	int error;

	if (uap->set && (error = copyin ((caddr_t) uap->set, (caddr_t) &nmask, sizeof (nmask))))
		nmask = 0;
	/*
	 * When returning from sigsuspend, we want the old mask to be restored
	 * after the signal handler has finished.  Thus, we save it here and set
	 * a flag to indicate this.
	 */
	u.u_oldmask = p->p_sigmask;
	u.u_psflags |= SAS_OLDMASK;
	p->p_sigmask = nmask &~ sigcantmask;
	while (tsleep((caddr_t)&u, PPAUSE|PCATCH, 0) == 0)
		;
	/* always return EINTR rather than ERESTART */
	u.u_error = EINTR;
}

void
sigaltstack()
{
	register struct a {
		struct sigaltstack * nss;
		struct sigaltstack * oss;
	} *uap = (struct a *)u.u_arg;
	struct sigaltstack ss;
	int error = 0;

	if ((u.u_psflags & SAS_ALTSTACK) == 0)
		u.u_sigstk.ss_flags |= SA_DISABLE;
	if (uap->oss && (error = copyout((caddr_t)&u.u_sigstk,
	    (caddr_t)uap->oss, sizeof (struct sigaltstack))))
		goto out;
	if (uap->nss == 0)
		goto out;
	error = copyin ((caddr_t) uap->nss, (caddr_t) &ss, sizeof(ss));
	if (error != 0)
		goto out;
	if (ss.ss_flags & SA_DISABLE) {
		if (u.u_sigstk.ss_flags & SA_ONSTACK)
			{
			error = EINVAL;
			goto out;
			}
		u.u_psflags &= ~SAS_ALTSTACK;
		u.u_sigstk.ss_flags = ss.ss_flags;
		goto out;
	}
	if (ss.ss_size < MINSIGSTKSZ)
		{
		error = ENOMEM;
		goto out;
		}
	u.u_psflags |= SAS_ALTSTACK;
	u.u_sigstk = ss;
out:
	u.u_error = error;
}

void
sigwait()
{
	register struct a {
		sigset_t *set;
		int *sig;
	} *uap = (struct a *)u.u_arg;
	sigset_t wanted, sigsavail;
	register struct proc *p = u.u_procp;
	int	signo, error;

	if (uap->set == 0 || uap->sig == 0) {
		error = EINVAL;
		goto out;
	}
	error = copyin ((caddr_t) uap->set, (caddr_t) &wanted, sizeof (sigset_t));
	if (error)
		goto out;

	wanted |= sigcantmask;
	while ((sigsavail = (wanted & p->p_sig)) == 0)
		tsleep ((caddr_t) &u.u_signal[0], PPAUSE | PCATCH, 0);

	if (sigsavail & sigcantmask) {
		error = EINTR;
		goto out;
	}

	signo = ffs(sigsavail);
	p->p_sig &= ~sigmask(signo);
	error = copyout ((caddr_t) &signo, (caddr_t) uap->sig, sizeof (int));
out:
	u.u_error = error;
}
