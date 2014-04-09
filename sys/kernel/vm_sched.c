/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "user.h"
#include "proc.h"
#include "vm.h"
#include "kernel.h"
#include "systm.h"
#include "debug.h"

#define	MINFINITY	-32767		/* minus infinity */

int	maxslp = MAXSLP;
char	runin;                          /* scheduling flag */
char	runout;                         /* scheduling flag */

/*
 * The main loop of the scheduling (swapping) process.
 * The basic idea is:
 *	see if anyone wants to be swapped in
 *	swap out processes until there is room
 *	swap him in
 *	repeat
 * The runout flag is set whenever someone is swapped out.  Sched sleeps on
 * it awaiting work.  Sched sleeps on runin whenever it cannot find enough
 * core (by swapping out or otherwise) to fit the selected swapped process.
 * It is awakened when the core situation changes and in any case once per
 * second.
 */
void
sched()
{
	register struct proc *rp;
	struct proc *swapped_out = 0, *in_core = 0;
	register int out_time, rptime;
    int s __attribute__((unused));

	for (;;) {
	        /* Perform swap-out/swap-in action. */
		s=spl0();
		if (in_core)
                        swapout (in_core, X_FREECORE, X_OLDSIZE, X_OLDSIZE);
		if (swapped_out)
                        swapin (swapped_out);
		s=splhigh();
		in_core = 0;
		swapped_out = 0;

                /* Find user to swap in; of users ready,
                 * select one out longest. */
		out_time = -20000;
		for (rp = allproc; rp; rp = rp->p_nxt) {
			if (rp->p_stat != SRUN || (rp->p_flag & SLOAD))
				continue;
			rptime = rp->p_time - rp->p_nice * 8;

			/*
			 * Always bring in parents ending a vfork,
			 * to avoid deadlock
			 */
			if (rptime > out_time || (rp->p_flag & SVFPRNT)) {
				swapped_out = rp;
				out_time = rptime;
				if (rp->p_flag & SVFPRNT)
					break;
			}
		}

		/* If there is no one there, wait. */
		if (! swapped_out) {
			++runout;
            //SETVAL(0);
			sleep ((caddr_t) &runout, PSWP);
			continue;
		}

        //SETVAL(swapped_out->p_pid);

		/*
		 * Look around for somebody to swap out.
		 * There may be only one non-system loaded process.
		 */
		for (rp = allproc; rp != NULL; rp = rp->p_nxt) {
			if (rp->p_stat != SZOMB &&
			    (rp->p_flag & (SSYS | SLOAD)) == SLOAD) {
			        in_core = rp;
				break;
                        }
		}
                if (! in_core) {
                        /* In-core memory is empty. */
			continue;
                }

                /*
                 * Swap found user out if sleeping interruptibly, or if he has spent at
                 * least 1 second in core and the swapped-out process has spent at
                 * least 2 seconds out.  Otherwise wait a bit and try again.
                 */
                if (! (in_core->p_flag & SLOCK) &&
                    (in_core->p_stat == SSTOP ||
                     (in_core->p_stat == SSLEEP && (in_core->p_flag & P_SINTR)) ||
                     ((in_core->p_stat == SRUN || in_core->p_stat == SSLEEP) &&
                      out_time >= 2 &&
                      in_core->p_time + in_core->p_nice >= 1)))
                {
                        /* Swap out in-core process. */
                        in_core->p_flag &= ~SLOAD;
                        if (in_core->p_stat == SRUN)
                                remrq (in_core);
                } else {
                        /* Nothing to swap in/out. */
                        in_core = 0;
                        swapped_out = 0;
                        ++runin;
                        sleep ((caddr_t) &runin, PSWP);
                }
	}
}

/*
 * Count up various things once a second
 */
void
vmmeter()
{
#ifdef UCB_METER
	register u_short *cp, *rp;
	register long *sp;

	ave(avefree, freemem, 5);
	ave(avefree30, freemem, 30);
	cp = &cnt.v_first;
        rp = &rate.v_first;
        sp = &sum.v_first;
	while (cp <= &cnt.v_last) {
		ave(*rp, *cp, 5);
		*sp += *cp;
		*cp = 0;
		rp++, cp++, sp++;
	}
#endif

	if (time.tv_sec % 5 == 0) {
		vmtotal();
#ifdef UCB_METER
		rate.v_swpin = cnt.v_swpin;
		sum.v_swpin += cnt.v_swpin;
		cnt.v_swpin = 0;
		rate.v_swpout = cnt.v_swpout;
		sum.v_swpout += cnt.v_swpout;
		cnt.v_swpout = 0;
#endif
	}
}

/*
 * Compute Tenex style load average.  This code is adapted from similar code
 * by Bill Joy on the Vax system.  The major change is that we avoid floating
 * point since not all pdp-11's have it.  This makes the code quite hard to
 * read - it was derived with some algebra.
 *
 * "floating point" numbers here are stored in a 16 bit short, with 8 bits on
 * each side of the decimal point.  Some partial products will have 16 bits to
 * the right.
 */
static void
loadav (avg, n)
	register short	*avg;
	register int	n;
{
	register int	i;
	static const long cexp[3] = {
		0353,	/* 256 * exp(-1/12)  */
		0373,	/* 256 * exp(-1/60)  */
		0376,	/* 256 * exp(-1/180) */
	};

	for (i = 0; i < 3; i++)
		avg[i] = (cexp[i] * (avg[i]-(n<<8)) + (((long)n)<<16)) >> 8;
}

void
vmtotal()
{
	register struct proc *p;
	register int nrun = 0;
#ifdef UCB_METER
	total.t_vmtxt = 0;
	total.t_avmtxt = 0;
	total.t_rmtxt = 0;
	total.t_armtxt = 0;
	total.t_vm = 0;
	total.t_avm = 0;
	total.t_rm = 0;
	total.t_arm = 0;
	total.t_rq = 0;
	total.t_dw = 0;
	total.t_sl = 0;
	total.t_sw = 0;
#endif
	for (p = allproc; p != NULL; p = p->p_nxt) {
		if (p->p_flag & SSYS)
			continue;
		if (p->p_stat) {
#ifdef UCB_METER
			if (p->p_stat != SZOMB) {
				total.t_vm += p->p_dsize + p->p_ssize + USIZE;
				if (p->p_flag & SLOAD)
					total.t_rm += p->p_dsize + p->p_ssize
					    + USIZE;
			}
#endif
			switch (p->p_stat) {

			case SSLEEP:
			case SSTOP:
				if (!(p->p_flag & P_SINTR) && p->p_stat == SSLEEP)
					nrun++;
#ifdef UCB_METER
				if (p->p_flag & SLOAD) {
					if	(!(p->p_flag & P_SINTR))
						total.t_dw++;
					else if (p->p_slptime < maxslp)
						total.t_sl++;
				} else if (p->p_slptime < maxslp)
					total.t_sw++;
				if (p->p_slptime < maxslp)
					goto active;
#endif
				break;

			case SRUN:
			case SIDL:
				nrun++;
#ifdef UCB_METER
				if (p->p_flag & SLOAD)
					total.t_rq++;
				else
					total.t_sw++;
active:
				total.t_avm += p->p_dsize + p->p_ssize + USIZE;
				if (p->p_flag & SLOAD)
					total.t_arm += p->p_dsize + p->p_ssize
					    + USIZE;
#endif
				break;
			}
		}
	}
#ifdef UCB_METER
	total.t_vm += total.t_vmtxt;
	total.t_avm += total.t_avmtxt;
	total.t_rm += total.t_rmtxt;
	total.t_arm += total.t_armtxt;
	total.t_free = avefree;
#endif
	loadav (avenrun, nrun);
}
