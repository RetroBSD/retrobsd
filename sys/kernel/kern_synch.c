/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "user.h"
#include "proc.h"
#include "buf.h"
#include "signal.h"
#include "signalvar.h"
#include "vm.h"
#include "kernel.h"
#include "systm.h"

#define	SQSIZE	16              /* Must be power of 2 */

#define	HASH(x)	(((int)x >> 5) & (SQSIZE - 1))
#define	SCHMAG	8/10

struct	proc *slpque[SQSIZE];

int	runrun;			/* scheduling flag */
char	curpri;			/* more scheduling */

/*
 * Recompute process priorities, once a second
 */
void
schedcpu (caddr_t arg)
{
	register struct proc *p;
	register int a;

	wakeup((caddr_t)&lbolt);
	for (p = allproc; p != NULL; p = p->p_nxt) {
		if (p->p_time != 127)
			p->p_time++;
		/*
		 * this is where 2.11 does its real time alarms.  4.X uses
		 * timeouts, since it offers better than second resolution.
		 * Putting it here allows us to continue using use an int
		 * to store the number of ticks in the callout structure,
		 * since the kernel never has a timeout of greater than
		 * around 9 minutes.
		 */
		if (p->p_realtimer.it_value && !--p->p_realtimer.it_value) {
			psignal(p, SIGALRM);
			p->p_realtimer.it_value = p->p_realtimer.it_interval;
		}
		if (p->p_stat == SSLEEP || p->p_stat == SSTOP)
			if (p->p_slptime != 127)
				p->p_slptime++;
		if (p->p_slptime > 1)
			continue;
		a = (p->p_cpu & 0377) * SCHMAG + p->p_nice;
		if (a < 0)
			a = 0;
		if (a > 255)
			a = 255;
		p->p_cpu = a;
		if (p->p_pri >= PUSER)
			setpri(p);
	}
	vmmeter();
	if (runin != 0) {
		runin = 0;
		wakeup((caddr_t)&runin);
	}
	++runrun;			/* swtch at least once a second */
	timeout (schedcpu, (caddr_t) 0, hz);
}

/*
 * Recalculate the priority of a process after it has slept for a while.
 */
void
updatepri(p)
	register struct proc *p;
{
	register int a = p->p_cpu & 0377;

	p->p_slptime--;		/* the first time was done in schedcpu */
	while (a && --p->p_slptime)
		a = (SCHMAG * a) /* + p->p_nice */;
	if (a < 0)
		a = 0;
	if (a > 255)
		a = 255;
	p->p_cpu = a;
	(void) setpri(p);
}

/*
 * Implement timeout for tsleep above.  If process hasn't been awakened
 * (p_wchan non zero) then set timeout flag and undo the sleep.  If proc
 * is stopped just unsleep so it will remain stopped.
 */
static void
endtsleep (p)
	register struct proc *p;
{
	register int	s;

	s = splhigh();
	if (p->p_wchan) {
		if (p->p_stat == SSLEEP)
			setrun(p);
		else
			unsleep(p);
		p->p_flag |= P_TIMEOUT;
	}
	splx(s);
}

/*
 * General sleep call "borrowed" from 4.4BSD - the 'wmesg' parameter was
 * removed due to data space concerns.  Sleeps at most timo/hz seconds
 * 0 means no timeout). NOTE: timeouts in 2.11BSD use a signed int and
 * thus can be at most 32767 'ticks' or about 540 seconds in the US with
 * 60hz power (~650 seconds if 50hz power is being used).
 *
 * If 'pri' includes the PCATCH flag signals are checked before and after
 * sleeping otherwise  signals are not checked.   Returns 0 if a wakeup was
 * done, EWOULDBLOCK if the timeout expired, ERESTART if the current system
 * call should be restarted, and EINTR if the system call should be
 * interrupted and EINTR returned to the user process.
 */
int
tsleep (ident, priority, timo)
	caddr_t	ident;
	int	priority;
	u_int	timo;
{
	register struct proc *p = u.u_procp;
	register struct proc **qp;
	int	s;
	int	sig, catch = priority & PCATCH;

	s = splhigh();
	if (panicstr) {
		/*
		 * After a panic just give interrupts a chance then just return.  Don't
		 * run any other procs (or panic again below) in case this is the idle
		 * process and already asleep.  The splnet should be spl0 if the network
		 * was being used but for now avoid network interrupts that might cause
		 * another panic.
		 */
		(void) splnet();
		noop();
		splx(s);
		return(0);
	}
#ifdef	DIAGNOSTIC
	if (ident == NULL || p->p_stat != SRUN)
		panic("tsleep");
#endif
	p->p_wchan = ident;
	p->p_slptime = 0;
	p->p_pri = priority & PRIMASK;
	qp = &slpque[HASH(ident)];
	p->p_link = *qp;
	*qp = p;
	if (timo)
		timeout (endtsleep, (caddr_t)p, timo);
	/*
	 * We put outselves on the sleep queue and start the timeout before calling
	 * CURSIG as we could stop there and a wakeup or a SIGCONT (or both) could
	 * occur while we were stopped.  A SIGCONT would cause us to be marked SSLEEP
	 * without resuming us thus we must be ready for sleep when CURSIG is called.
	 * If the wakeup happens while we're stopped p->p_wchan will be 0 upon
	 * return from CURSIG.
	 */
	if (catch) {
		p->p_flag |= P_SINTR;
		sig = CURSIG(p);
		if (sig) {
			if (p->p_wchan)
				unsleep(p);
			p->p_stat = SRUN;
			goto resume;
		}
		if (p->p_wchan == 0) {
			catch = 0;
			goto resume;
		}
	} else
		sig = 0;

	p->p_stat = SSLEEP;
        if (p != &proc[0])
                wakeup((caddr_t) &runin);
	u.u_ru.ru_nvcsw++;
	swtch();
resume:
	splx(s);
	p->p_flag &= ~P_SINTR;
	if (p->p_flag & P_TIMEOUT) {
		p->p_flag &= ~P_TIMEOUT;
		if (sig == 0)
			return(EWOULDBLOCK);
	} else if (timo)
		untimeout (endtsleep, (caddr_t)p);
	if (catch && (sig != 0 || (sig = CURSIG(p)))) {
		if (u.u_sigintr & sigmask(sig))
			return(EINTR);
		return(ERESTART);
	}
	return(0);
}

/*
 * Give up the processor till a wakeup occurs on chan, at which time the
 * process enters the scheduling queue at priority pri.
 *
 * This routine was rewritten to use 'tsleep'.  The  old behaviour of sleep
 * being interruptible (if 'pri>PZERO') is emulated by setting PCATCH and
 * then performing the 'longjmp' if the return value of 'tsleep' is
 * ERESTART.
 *
 * Callers of this routine must be prepared for premature return, and check
 * that the reason for sleeping has gone away.
 */
void
sleep (chan, pri)
	caddr_t chan;
	int pri;
{
	register int priority = pri;

	if (pri > PZERO)
		priority |= PCATCH;

	u.u_error = tsleep (chan, priority, 0);
	/*
	 * sleep does not return anything.  If it was a non-interruptible sleep _or_
	 * a successful/normal sleep (one for which a wakeup was done) then return.
	 */
	if ((priority & PCATCH) == 0 || (u.u_error == 0))
		return;
	/*
	 * XXX - compatibility uglyness.
	 *
	 * The tsleep() above will leave one of the following in u_error:
	 *
	 * 0 - a wakeup was done, this is handled above
	 * EWOULDBLOCK - since no timeout was passed to tsleep we will not see this
	 * EINTR - put into u_error for trap.c to find (interrupted syscall)
	 * ERESTART - system call to be restared
	 */
	longjmp (u.u_procp->p_addr, &u.u_qsave);
	/*NOTREACHED*/
}

/*
 * Remove a process from its wait queue
 */
void
unsleep (p)
	register struct proc *p;
{
	register struct proc **hp;
	register int s;

	s = splhigh();
	if (p->p_wchan) {
		hp = &slpque[HASH(p->p_wchan)];
		while (*hp != p)
			hp = &(*hp)->p_link;
		*hp = p->p_link;
		p->p_wchan = 0;
	}
	splx(s);
}

/*
 * Wake up all processes sleeping on chan.
 */
void
wakeup (chan)
	register caddr_t chan;
{
	register struct proc *p, **q;
	struct proc **qp;
	int s;

	/*
	 * Since we are called at interrupt time, must insure normal
	 * kernel mapping to access proc.
	 */
	s = splclock();
	qp = &slpque[HASH(chan)];
restart:
	for (q = qp; (p = *q); ) {
		if (p->p_stat != SSLEEP && p->p_stat != SSTOP)
			panic("wakeup");
		if (p->p_wchan==chan) {
			p->p_wchan = 0;
			*q = p->p_link;
			if (p->p_stat == SSLEEP) {
				/* OPTIMIZED INLINE EXPANSION OF setrun(p) */
				if (p->p_slptime > 1)
					updatepri(p);
				p->p_slptime = 0;
				p->p_stat = SRUN;
				if (p->p_flag & SLOAD)
					setrq(p);
				/*
				 * Since curpri is a usrpri,
				 * p->p_pri is always better than curpri.
				 */
				runrun++;
				if (! (p->p_flag & SLOAD)) {
					if (runout != 0) {
						runout = 0;
						wakeup((caddr_t)&runout);
					}
				}
				/* END INLINE EXPANSION */
				goto restart;
			}
			p->p_slptime = 0;
		} else
			q = &p->p_link;
	}
	splx(s);
}

/*
 * Set the process running;
 * arrange for it to be swapped in if necessary.
 */
void
setrun (p)
	register struct proc *p;
{
	register int s;

	s = splhigh();
	switch (p->p_stat) {
	case 0:
	case SWAIT:
	case SRUN:
	case SZOMB:
	default:
		panic("setrun");

	case SSTOP:
	case SSLEEP:
		unsleep(p);		/* e.g. when sending signals */
		break;

	case SIDL:
		break;
	}
	if (p->p_slptime > 1)
		updatepri(p);
	p->p_stat = SRUN;
	if (p->p_flag & SLOAD)
		setrq(p);
	splx(s);
	if (p->p_pri < curpri)
		runrun++;
	if (! (p->p_flag & SLOAD)) {
		if (runout != 0) {
			runout = 0;
			wakeup((caddr_t)&runout);
		}
	}
}

/*
 * Set user priority.
 * The rescheduling flag (runrun)
 * is set if the priority is better
 * than the currently running process.
 */
int
setpri (pp)
	register struct proc *pp;
{
	register int p;

	p = (pp->p_cpu & 0377)/16;
	p += PUSER + pp->p_nice;
	if (p > 127)
		p = 127;
	if (p < curpri)
		runrun++;
	pp->p_pri = p;
	return (p);
}

/*
 * This routine is called to reschedule the CPU.  If the calling process is
 * not in RUN state, arrangements for it to restart must have been made
 * elsewhere, usually by calling via sleep.  There is a race here.  A process
 * may become ready after it has been examined.  In this case, idle() will be
 * called and will return in at most 1hz time, e.g. it's not worth putting an
 * spl() in.
 */
void
swtch()
{
	register struct proc *p, *q;
	register int n;
	struct proc *pp, *pq;
	int s;

#ifdef UCB_METER
	cnt.v_swtch++;
#endif
	/* If not the idle process, resume the idle process. */
	if (u.u_procp != &proc[0]) {
		if (setjmp (&u.u_rsave)) {
			/* Returned from swapper to user process. */
			return;
		}
		/* Switch from user process to swapper. */
		longjmp (proc[0].p_addr, &u.u_qsave);
	}
	/*
	 * The first save returns nonzero when proc 0 is resumed
	 * by another process (above); then the second is not done
	 * and the process-search loop is entered.
	 */
	if (setjmp (&u.u_qsave)) {
		/* Returned from user process. */
		goto loop;
	}
	/*
	 * The first save returns 0 when swtch is called in proc 0
	 * from sched().  The second save returns 0 immediately, so
	 * in this case too the process-search loop is entered.
	 * Thus when proc 0 is awakened by being made runnable, it will
	 * find itself and resume itself at rsave, and return to sched().
	 */
	if (setjmp (&u.u_rsave)) {
		/* Swapper resumed by itself. */
		return;
	}
loop:
	s = splhigh();
	noproc = 0;
	runrun = 0;
#ifdef DIAGNOSTIC
	for (p = qs; p; p = p->p_link)
		if (p->p_stat != SRUN)
			panic ("swtch SRUN");
#endif
	pp = NULL;
	q = NULL;
	n = 128;
	/*
	 * search for highest-priority runnable process
	 */
	pq = 0;
	for (p = qs; p; p = p->p_link) {
		if (p->p_flag & SLOAD && p->p_pri < n) {
			pp = p;
			pq = q;
			n = p->p_pri;
		}
		q = p;
	}
	/*
	 * if no process is runnable, idle.
	 */
	p = pp;
	if (p == NULL) {
		idle();
		goto loop;
	}
	if (pq)
		pq->p_link = p->p_link;
	else
		qs = p->p_link;
	curpri = n;
	splx(s);
	/*
	 * the rsave (ssave) contents are interpreted
	 * in the new address space
	 */
	n = p->p_flag & SSWAP;
	p->p_flag &= ~SSWAP;
	longjmp (p->p_addr, n ? &u.u_ssave : &u.u_rsave);
}

/*
 * Put the process into the run queue.
 */
void
setrq (p)
	register struct proc *p;
{
	register int s;

	s = splhigh();
#ifdef DIAGNOSTIC
	{			/* see if already on the run queue */
		register struct proc *q;

		for (q = qs;q != NULL;q = q->p_link)
			if (q == p)
				panic("setrq");
	}
#endif
	p->p_link = qs;
	qs = p;
	splx(s);
}

/*
 * Remove runnable job from run queue.  This is done when a runnable job
 * is swapped out so that it won't be selected in swtch().  It will be
 * reinserted in the qs with setrq when it is swapped back in.
 */
void
remrq (p)
	register struct proc *p;
{
	register struct proc *q;
	register int s;

	s = splhigh();
	if (p == qs)
		qs = p->p_link;
	else {
		for (q = qs; q; q = q->p_link)
			if (q->p_link == p) {
				q->p_link = p->p_link;
				goto done;
			}
			panic("remrq");
	}
done:
	splx(s);
}
