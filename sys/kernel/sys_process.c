/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "systm.h"
#include "user.h"
#include "proc.h"
#include "inode.h"
#include "vm.h"
#include "ptrace.h"

/*
 * sys-trace system call.
 */
void
ptrace()
{
	register struct proc *p;
	register struct a {
		int	req;
		int	pid;
		int	*addr;
		int	data;
	} *uap;

	uap = (struct a *)u.u_arg;
	if (uap->req <= 0) {
		u.u_procp->p_flag |= P_TRACED;
		return;
	}
	p = pfind(uap->pid);
	if (p == 0 || p->p_stat != SSTOP || p->p_ppid != u.u_procp->p_pid ||
	    !(p->p_flag & P_TRACED)) {
		u.u_error = ESRCH;
		return;
	}
	while (ipc.ip_lock)
		sleep((caddr_t)&ipc, PZERO);
	ipc.ip_lock = p->p_pid;
	ipc.ip_data = uap->data;
	ipc.ip_addr = uap->addr;
	ipc.ip_req = uap->req;
	p->p_flag &= ~P_WAITED;
	setrun(p);
	while (ipc.ip_req > 0)
		sleep((caddr_t)&ipc, PZERO);
	u.u_rval = ipc.ip_data;
	if (ipc.ip_req < 0)
		u.u_error = EIO;
	ipc.ip_lock = 0;
	wakeup((caddr_t)&ipc);
}

/*
 * Code that the child process
 * executes to implement the command
 * of the parent process in tracing.
 */
int
procxmt()
{
	register int i, *p;

	if (ipc.ip_lock != u.u_procp->p_pid)
		return(0);
	u.u_procp->p_slptime = 0;
	i = ipc.ip_req;
	ipc.ip_req = 0;
	wakeup ((caddr_t)&ipc);
	switch (i) {

	/* read user I */
	case PT_READ_I:

	/* read user D */
	case PT_READ_D:
		if (baduaddr ((caddr_t) ipc.ip_addr))
			goto error;
		ipc.ip_data = *(int*) ipc.ip_addr;
		break;

	/* read u */
	case PT_READ_U:
		i = (int) ipc.ip_addr;
		if (i < 0 || i >= USIZE)
			goto error;
		ipc.ip_data = ((unsigned*)&u) [i/sizeof(int)];
		break;

	/* write user I */
	case PT_WRITE_I:
	/* write user D */
	case PT_WRITE_D:
		if (baduaddr ((caddr_t) ipc.ip_addr))
			goto error;
		*(int*) ipc.ip_addr = ipc.ip_data;
		break;

	/* write u */
	case PT_WRITE_U:
		i = (int)ipc.ip_addr;
		p = (int*)&u + i/sizeof(int);
		for (i=0; i<FRAME_WORDS; i++)
			if (p == &u.u_frame[i])
				goto ok;
                goto error;
ok:
		*p = ipc.ip_data;
		break;

	/* set signal and continue */
	/* one version causes a trace-trap */
	case PT_STEP:
		/* Use Status.RP bit to indicate a single-step request. */
		u.u_frame [FRAME_STATUS] |= ST_RP;
		/* FALL THROUGH TO ... */
	case PT_CONTINUE:
		if ((int)ipc.ip_addr != 1)
			u.u_frame [FRAME_PC] = (int)ipc.ip_addr;
		if (ipc.ip_data > NSIG)
			goto error;
		u.u_procp->p_ptracesig = ipc.ip_data;
		return(1);

	/* force exit */
	case PT_KILL:
		exit(u.u_procp->p_ptracesig);
		/*NOTREACHED*/

	default:
error:
		ipc.ip_req = -1;
	}
	return(0);
}
