/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/systm.h>

struct proc *pidhash [PIDHSZ];
struct proc *freeproc, *zombproc, *allproc, *qs; /* lists of procs in various states */

/*
 * Is p an inferior of the current process?
 */
int
inferior(struct proc *p)
{
    for (; p != u.u_procp; p = p->p_pptr)
        if (p->p_ppid == 0)
            return (0);
    return (1);
}

/*
 * Find a process by pid.
 */
struct proc *
pfind (int pid)
{
    register struct proc *p = pidhash [PIDHASH(pid)];

    for (; p; p = p->p_hash)
        if (p->p_pid == pid)
            return (p);
    return ((struct proc *)0);
}

/*
 * init the process queues
 */
void
pqinit()
{
    register struct proc *p;

    /*
     * most procs are initially on freequeue
     *  nb: we place them there in their "natural" order.
     */

    freeproc = NULL;
    for (p = proc+NPROC; --p > proc; freeproc = p)
        p->p_nxt = freeproc;

    /*
     * but proc[0] is special ...
     */

    allproc = p;
    p->p_nxt = NULL;
    p->p_prev = &allproc;

    zombproc = NULL;
}
