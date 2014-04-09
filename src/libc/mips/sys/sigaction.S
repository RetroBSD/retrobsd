/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * error = sigaction(sig, vec, ovec)
 *	int		error, sig;
 *	struct sigaction *vec, *ovec;
 *
 * We pass one additional parameter to the sigaction sys call: the address of
 * the "Trampoline Code", sigtramp - the code that handles saving and restoring
 * register context and so on for signals.  On the VAX-11 under BSD4.3 it isn't
 * necessary to pass this address since the trampoline code is stored in the
 * user structure in u.u_pcb.pcb_sigc at a known address in user space.  It
 * really doesn't introduce much extra overhead, so our method for doing it on
 * a PDP-11 is alright too.
 */
#include "SYS.h"

ENTRY(sigaction)
                la      $a3, sigtramp           # arg4: address of sigtramp

		.set	noreorder
		syscall	SYS_sigaction
		lui	$t1, %hi(errno)		# return here on error
		sw      $t0, %lo(errno)($t1)
		.set	reorder

		jr	$ra			# return here on success

/*
 * sigtramp - Signal "Trampoline Code"
 *
 * This code is transfered to by the kernel when a signal is delivered to a
 * process.  In general, the idea is that sigtramp saves the process' register
 * context and then vectors on to the real signal action routine.  Upon return
 * from the signal action routine sigtramp restores the process' register
 * context and performs a sigreturn.
 *
 * In the case of the PDP-11, the kernel will have already saved r0 and r1 for
 * sigtramp in a sigcontext structure it passes to us.  Sigtramp vectors onto
 * the signal action routine whose address has been left in r0 by the kernel
 * (sigtramp assumes the signal action routine will save any other registers
 * it uses (as all C routines will)).  Upon return from the signal action
 * routine, sigtramp will execute a sigreturn with the sigcontext structure
 * given to us by the kernel.
 *
 * When the kernel transfers control to sigtramp the stack looks like:
 *
 *	-------------------------
 *	| sigcontext structure	| SIG_SC = sp + 8
 *	|-----------------------|
 *	| unused		|
 *	|-----------------------|
 *	| $a2: ptr to sigcontext|
 *	|-----------------------|
 *	| $a1: code		|
 *	|-----------------------|
 *sp ->	| $a0: signal number	|
 *	-------------------------
 *
 * The important features of this as far as sigtramp is concerned are:
 * 1.	The fact that the signal number, signal code, and signal context
 *	pointer are already set up as parameters to the signal action
 *	routine.
 * 2.   There's no need to save r0 & r1 because the kernel's already saved
 *      them for us in the sigcontext structure (C routines save all
 *      registers except r0 & r1 automatically).
 *
 * Note that the stack offset SIG_SC will NOT have to be recomputed if the
 * sigcontext structure changes.
 */
sigtramp:
                syscall	SYS_sigreturn		# attempt the sigreturn
                syscall	SYS_exit		# die if the sigreturn fails ...
