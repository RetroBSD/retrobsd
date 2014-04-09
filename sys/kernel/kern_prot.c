/*
 * System calls related to processes and protection.
 *
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "user.h"
#include "proc.h"
#include "systm.h"

void
getpid()
{
	u.u_rval = u.u_procp->p_pid;
}

void
getppid()
{
	u.u_rval = u.u_procp->p_ppid;
}

void
getpgrp()
{
	register struct a {
		int	pid;
	} *uap = (struct a *)u.u_arg;
	register struct proc *p;

	if (uap->pid == 0)		/* silly... */
		uap->pid = u.u_procp->p_pid;
	p = pfind(uap->pid);
	if (p == 0) {
		u.u_error = ESRCH;
		return;
	}
	u.u_rval = p->p_pgrp;
}

void
getuid()
{
	u.u_rval = u.u_ruid;
}

void
geteuid()
{
	u.u_rval = u.u_uid;
}

void
getgid()
{
	u.u_rval = u.u_rgid;
}

void
getegid()
{
	u.u_rval = u.u_groups[0];
}

/*
 * getgroups and setgroups differ from 4.X because the VAX stores group
 * entries in the user structure as shorts and has to convert them to ints.
 */
void
getgroups()
{
	register struct	a {
		u_int	gidsetsize;
		int	*gidset;
	} *uap = (struct a *)u.u_arg;
	register gid_t *gp;

	for (gp = &u.u_groups[NGROUPS]; gp > u.u_groups; gp--)
		if (gp[-1] != NOGROUP)
			break;
	if (uap->gidsetsize < gp - u.u_groups) {
		u.u_error = EINVAL;
		return;
	}
	uap->gidsetsize = gp - u.u_groups;
	u.u_error = copyout((caddr_t)u.u_groups, (caddr_t)uap->gidset,
	    uap->gidsetsize * sizeof(u.u_groups[0]));
	if (u.u_error)
		return;
	u.u_rval = uap->gidsetsize;
}

void
setpgrp()
{
	register struct proc *p;
	register struct a {
		int	pid;
		int	pgrp;
	} *uap = (struct a *)u.u_arg;

	if (uap->pid == 0)		/* silly... */
		uap->pid = u.u_procp->p_pid;
	p = pfind(uap->pid);
	if (p == 0) {
		u.u_error = ESRCH;
		return;
	}
	/* need better control mechanisms for process groups */
	if (p->p_uid != u.u_uid && u.u_uid && !inferior(p)) {
		u.u_error = EPERM;
		return;
	}
	p->p_pgrp = uap->pgrp;
}

void
setgroups()
{
	register struct	a {
		u_int	gidsetsize;
		int	*gidset;
	} *uap = (struct a *)u.u_arg;
	register gid_t *gp;

	if (!suser())
		return;
	if (uap->gidsetsize > sizeof (u.u_groups) / sizeof (u.u_groups[0])) {
		u.u_error = EINVAL;
		return;
	}
	u.u_error = copyin((caddr_t)uap->gidset, (caddr_t)u.u_groups,
	uap->gidsetsize * sizeof (u.u_groups[0]));
	if (u.u_error)
		return;
	for (gp = &u.u_groups[uap->gidsetsize]; gp < &u.u_groups[NGROUPS]; gp++)
		*gp = NOGROUP;
}

/*
 * Check if gid is a member of the group set.
 */
int
groupmember(gid)
	gid_t gid;
{
	register gid_t *gp;

	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS] && *gp != NOGROUP; gp++)
		if (*gp == gid)
			return (1);
	return (0);
}
