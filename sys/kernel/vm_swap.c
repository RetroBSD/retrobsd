/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "user.h"
#include "proc.h"
#include "map.h"
#include "buf.h"
#include "systm.h"
#include "vm.h"

/*
 * Swap a process in.
 * Allocate data and possible text separately.  It would be better
 * to do largest first.  Text, data, and stack are allocated in
 * that order, as that is likely to be in order of size.
 * U area goes into u0 buffer.
 */
void
swapin (p)
	register struct proc *p;
{
	size_t daddr = USER_DATA_START;
	size_t saddr = USER_DATA_END - p->p_ssize;
	size_t uaddr = (size_t) &u0;

	if (p->p_dsize) {
		swap (p->p_daddr, daddr, p->p_dsize, B_READ);
		mfree (swapmap, btod (p->p_dsize), p->p_daddr);
	}
	if (p->p_ssize) {
		swap (p->p_saddr, saddr, p->p_ssize, B_READ);
		mfree (swapmap, btod (p->p_ssize), p->p_saddr);
	}
	swap (p->p_addr, uaddr, USIZE, B_READ);
	mfree (swapmap, btod (USIZE), p->p_addr);

	p->p_daddr = daddr;
	p->p_saddr = saddr;
	p->p_addr = uaddr;
	if (p->p_stat == SRUN)
		setrq (p);
	p->p_flag |= SLOAD;
	p->p_time = 0;
#ifdef UCB_METER
	cnt.v_swpin++;
#endif
}

/*
 * Swap out process p.
 * odata and ostack are the old data size and the stack size
 * of the process, and are supplied during core expansion swaps.
 * The freecore flag causes its core to be freed -- it may be
 * off when called to create an image for a child process
 * in newproc.
 *
 * panic: out of swap space
 */
void
swapout (p, freecore, odata, ostack)
	register struct proc *p;
	int freecore;
	register u_int odata, ostack;
{
	size_t a[3];

	if (odata == (u_int) X_OLDSIZE)
		odata = p->p_dsize;
	if (ostack == (u_int) X_OLDSIZE)
		ostack = p->p_ssize;
	if (malloc3 (swapmap, btod (p->p_dsize), btod (p->p_ssize),
	    btod (USIZE), a) == NULL)
		panic ("out of swap space");
	p->p_flag |= SLOCK;
	if (odata) {
		swap (a[0], p->p_daddr, odata, B_WRITE);
	}
	if (ostack) {
		swap (a[1], p->p_saddr, ostack, B_WRITE);
	}
	/*
	 * Increment u_ru.ru_nswap for process being tossed out of core.
	 * We can be called to swap out a process other than the current
	 * process, so we have to map in the victim's u structure briefly.
	 * Note, savekdsa6 *must* be a static, because we remove the stack
	 * in the next instruction.  The splclock is to prevent the clock
	 * from coming in and doing accounting for the wrong process, plus
	 * we don't want to come through here twice.  Why are we doing
	 * this, anyway?
	 */
	{
		int s;

		s = splclock();
		u.u_ru.ru_nswap++;
		splx (s);
	}
	swap (a[2], p->p_addr, USIZE, B_WRITE);
	p->p_daddr = a[0];
	p->p_saddr = a[1];
	p->p_addr = a[2];
	p->p_flag &= ~(SLOAD|SLOCK);
	p->p_time = 0;

#ifdef UCB_METER
	cnt.v_swpout++;
#endif
	if (runout) {
		runout = 0;
		wakeup ((caddr_t)&runout);
	}
}
