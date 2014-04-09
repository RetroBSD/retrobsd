/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "systm.h"
#include "signalvar.h"
#include "user.h"
#include "proc.h"
#include "buf.h"
#include "map.h"
#include "syslog.h"

//#define DIAGNOSTIC

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow trampoline code stored at u.u_sigtramp (as
 * specified by the user process) to call the user's real signal catch
 * routine, followed by sys sigreturn to the sigreturn routine below
 * (see lib/libc/mips/sys/sigaction.S).  After sigreturn resets the signal
 * mask, the stack, and the frame pointer, it returns to the user specified
 * pc and regs.
 */
void
sendsig (p, sig, mask)
	sig_t p;
	int sig;
	long mask;
{
	struct sigframe {
		int	sf_space [4];
		struct	sigcontext sf_sc;
	};
	register int *regs = u.u_frame;
	register struct sigframe *sfp;
	int oonstack;

#ifdef DIAGNOSTIC
	printf("(%u)sendsig %d, mask=%#x, handler=%#x, tramp=%#x\n",
                u.u_procp->p_pid, sig, mask, p, u.u_sigtramp);
#endif
	oonstack = u.u_sigstk.ss_flags & SA_ONSTACK;

	/*
	 * Allocate and validate space for the signal frame.
	 */
	if ((u.u_psflags & SAS_ALTSTACK) &&
	     ! (u.u_sigstk.ss_flags & SA_ONSTACK) &&
	    (u.u_sigonstack & sigmask(sig))) {
		sfp = (struct sigframe*) (u.u_sigstk.ss_base +
                        u.u_sigstk.ss_size);
		u.u_sigstk.ss_flags |= SA_ONSTACK;
	} else
		sfp = (struct sigframe*) regs [FRAME_SP];

        sfp--;
	if (! (u.u_sigstk.ss_flags & SA_ONSTACK)) {
                if ((caddr_t) sfp < (caddr_t) u.u_procp->p_daddr + u.u_dsize) {
                        /*
                         * Process has trashed its stack; give it an illegal
                         * instruction violation to halt it in its tracks.
                         */
                        fatalsig(SIGILL);
                        return;
                }
                if (u.u_procp->p_ssize < USER_DATA_END - (unsigned) sfp) {
                        u.u_procp->p_ssize = USER_DATA_END - (unsigned) sfp;
                        u.u_procp->p_saddr = (unsigned) sfp;
                        u.u_ssize = u.u_procp->p_ssize;
                }
        }

	/*
	 * Build the signal context to be used by sigreturn.
	 */
	sfp->sf_sc.sc_onstack = oonstack;
	sfp->sf_sc.sc_mask = mask;
        sfp->sf_sc.sc_r1  = regs [FRAME_R1];
        sfp->sf_sc.sc_r2  = regs [FRAME_R2];
        sfp->sf_sc.sc_r3  = regs [FRAME_R3];
        sfp->sf_sc.sc_r4  = regs [FRAME_R4];
        sfp->sf_sc.sc_r5  = regs [FRAME_R5];
        sfp->sf_sc.sc_r6  = regs [FRAME_R6];
        sfp->sf_sc.sc_r7  = regs [FRAME_R7];
        sfp->sf_sc.sc_r8  = regs [FRAME_R8];
        sfp->sf_sc.sc_r9  = regs [FRAME_R9];
        sfp->sf_sc.sc_r10 = regs [FRAME_R10];
        sfp->sf_sc.sc_r11 = regs [FRAME_R11];
        sfp->sf_sc.sc_r12 = regs [FRAME_R12];
        sfp->sf_sc.sc_r13 = regs [FRAME_R13];
        sfp->sf_sc.sc_r14 = regs [FRAME_R14];
        sfp->sf_sc.sc_r15 = regs [FRAME_R15];
        sfp->sf_sc.sc_r16 = regs [FRAME_R16];
        sfp->sf_sc.sc_r17 = regs [FRAME_R17];
        sfp->sf_sc.sc_r18 = regs [FRAME_R18];
        sfp->sf_sc.sc_r19 = regs [FRAME_R19];
        sfp->sf_sc.sc_r20 = regs [FRAME_R20];
        sfp->sf_sc.sc_r21 = regs [FRAME_R21];
        sfp->sf_sc.sc_r22 = regs [FRAME_R22];
        sfp->sf_sc.sc_r23 = regs [FRAME_R23];
        sfp->sf_sc.sc_r24 = regs [FRAME_R24];
        sfp->sf_sc.sc_r25 = regs [FRAME_R25];
        sfp->sf_sc.sc_gp  = regs [FRAME_GP];
        sfp->sf_sc.sc_sp  = regs [FRAME_SP];
        sfp->sf_sc.sc_fp  = regs [FRAME_FP];
        sfp->sf_sc.sc_ra  = regs [FRAME_RA];
        sfp->sf_sc.sc_lo  = regs [FRAME_LO];
        sfp->sf_sc.sc_hi  = regs [FRAME_HI];
        sfp->sf_sc.sc_pc  = regs [FRAME_PC];

        /* Call signal handler */
	regs [FRAME_R4] = sig;                  /* $a0 - signal number */
	regs [FRAME_R5] = u.u_code;             /* $a1 - code */
	regs [FRAME_R6] = (int) &sfp->sf_sc;    /* $a2 - address of sigcontext */
	regs [FRAME_RA] = (int) u.u_sigtramp;   /* $ra - sigtramp */
	regs [FRAME_SP] = (int) sfp;
	regs [FRAME_PC] = (int) p;
#ifdef DIAGNOSTIC
	printf("    ...call handler %p (sig=%d, code=%#x, context=%p)\n",
	    p, sig, u.u_code, &sfp->sf_sc);
	printf("    ...stack=%p, return to %p\n", sfp, u.u_sigtramp);
#endif
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and ps as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * ps to gain improper priviledges or to cause
 * a machine fault.
 */
void
sigreturn()
{
	register int *regs = u.u_frame;
	register struct sigcontext *scp =
                (struct sigcontext*) (regs [FRAME_SP] + 16);

#ifdef DIAGNOSTIC
	printf("(%u)sigreturn stack=%#x, context=%p\n",
                u.u_procp->p_pid, regs [FRAME_SP], scp);
#endif
	if (baduaddr ((caddr_t) scp) ||
            baduaddr ((caddr_t) scp + sizeof(*scp))) {
		u.u_error = EFAULT;
		return;
        }
	u.u_error = EJUSTRETURN;
	if (scp->sc_onstack & SA_ONSTACK)
		u.u_sigstk.ss_flags |= SA_ONSTACK;
	else
		u.u_sigstk.ss_flags &= ~SA_ONSTACK;
	u.u_procp->p_sigmask = scp->sc_mask & ~sigcantmask;

        /* Return from signal handler */
        regs [FRAME_R1] = scp->sc_r1;
        regs [FRAME_R2] = scp->sc_r2;
        regs [FRAME_R3] = scp->sc_r3;
        regs [FRAME_R4] = scp->sc_r4;
        regs [FRAME_R5] = scp->sc_r5;
        regs [FRAME_R6] = scp->sc_r6;
        regs [FRAME_R7] = scp->sc_r7;
        regs [FRAME_R8] = scp->sc_r8;
        regs [FRAME_R9] = scp->sc_r9;
        regs [FRAME_R10] = scp->sc_r10;
        regs [FRAME_R11] = scp->sc_r11;
        regs [FRAME_R12] = scp->sc_r12;
        regs [FRAME_R13] = scp->sc_r13;
        regs [FRAME_R14] = scp->sc_r14;
        regs [FRAME_R15] = scp->sc_r15;
        regs [FRAME_R16] = scp->sc_r16;
        regs [FRAME_R17] = scp->sc_r17;
        regs [FRAME_R18] = scp->sc_r18;
        regs [FRAME_R19] = scp->sc_r19;
        regs [FRAME_R20] = scp->sc_r20;
        regs [FRAME_R21] = scp->sc_r21;
        regs [FRAME_R22] = scp->sc_r22;
        regs [FRAME_R23] = scp->sc_r23;
        regs [FRAME_R24] = scp->sc_r24;
        regs [FRAME_R25] = scp->sc_r25;
        regs [FRAME_GP] = scp->sc_gp;
        regs [FRAME_SP] = scp->sc_sp;
        regs [FRAME_FP] = scp->sc_fp;
        regs [FRAME_RA] = scp->sc_ra;
        regs [FRAME_LO] = scp->sc_lo;
        regs [FRAME_HI] = scp->sc_hi;
        regs [FRAME_PC] = scp->sc_pc;
#ifdef DIAGNOSTIC
	printf("    ...to %#x, stack %#x\n", regs[FRAME_PC], regs[FRAME_SP]);
#endif
}
