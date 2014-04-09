/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 */
#include "defs.h"

char    *trapcom[MAXTRAP];

BOOL    trapflg[MAXTRAP] =
{
	0,
	0,      /* 1 hangup */
	0,      /* 2 interrupt */
	0,      /* 3 quit */
	0,      /* 4 illegal instr */
	0,      /* 5 trace trap */
	0,      /* 6 IOT */
	0,      /* 7 EMT */
	0,      /* 8 float pt. exp */
	0,      /* 9 kill */
	0,      /* 10 bus error */
	0,      /* 11 memory faults */
	0,      /* 12 bad sys call */
	0,      /* 13 bad pipe call */
	0,      /* 14 alarm */
	0,      /* 15 software termination */
	0,      /* 16 urgent */
	0,      /* 17 stop */
	0,      /* 18 tstp */
	0,      /* 19 cont */
	0,      /* 20 death or stop of child */
	0,      /* 21 ttin */
	0,      /* 22 ttou */
	0,      /* 23 tint */
};

void (*sigval[])() = {
	0,
	done,
	fault,
	fault,
	done,
	done,
	abort,
	done,
	done,
	SIG_DFL,
	done,
	done,
	done,
	done,
	fault,
	fault,
	done,
	SIG_DFL,
	SIG_DFL,
	SIG_DFL,
	done,
	SIG_DFL,
	SIG_DFL,
	SIG_DFL,
};

/* ========     fault handling routines    ======== */

void
fault(sig)
register int    sig;
{
	register int    flag;

	signal(sig, fault);
	if (sig == SIGSEGV)
	{
		if (setbrk(brkincr) == -1)
			error(nospace);
	}
	else if (sig == SIGALRM)
	{
		if (flags & waiting)
			done();
	}
	else
	{
		flag = (trapcom[sig] ? TRAPSET : SIGSET);
		trapnote |= flag;
		trapflg[sig] |= flag;
		if (sig == SIGINT)
			wasintr++;
	}
}

stdsigs()
{
	setsig(SIGHUP);
	setsig(SIGINT);
	ignsig(SIGQUIT);
	setsig(SIGILL);
	setsig(SIGTRAP);
	setsig(SIGIOT);
	setsig(SIGEMT);
	setsig(SIGFPE);
	setsig(SIGBUS);
	signal(SIGSEGV, fault);
	setsig(SIGSYS);
	setsig(SIGPIPE);
	setsig(SIGALRM);
	setsig(SIGTERM);
#ifdef SIGUSR1
	setsig(SIGUSR1);
#endif
#ifdef SIGUSR2
	setsig(SIGUSR2);
#endif
}

ignsig(n)
{
	register int    s, i;

	if ((i = n) == SIGSEGV)
	{
		clrsig(i);
		failed(badtrap, "cannot trap 11");
	}
	else if ((s = (signal(i, SIG_IGN) == SIG_IGN)) == 0)
	{
		trapflg[i] |= SIGMOD;
	}
	return(s);
}

getsig(n)
{
	register int    i;

	if (trapflg[i = n] & SIGMOD || ignsig(i) == 0)
		signal(i, fault);
}


setsig(n)
{
	register int    i;

	if (ignsig(i = n) == 0)
		signal(i, sigval[i]);
}

oldsigs()
{
	register int    i;
	register char   *t;

	i = MAXTRAP;
	while (i--)
	{
		t = trapcom[i];
		if (t == NIL || *t)
			clrsig(i);
		trapflg[i] = 0;
	}
	trapnote = 0;
}

clrsig(i)
int     i;
{
	free(trapcom[i]);
	trapcom[i] = NIL;
	if (trapflg[i] & SIGMOD)
	{
		trapflg[i] &= ~SIGMOD;
		signal(i, sigval[i]);
	}
}

/*
 * check for traps
 */
chktrap()
{
	register int    i = MAXTRAP;
	register char   *t;

	trapnote &= ~TRAPSET;
	while (--i)
	{
		if (trapflg[i] & TRAPSET)
		{
			trapflg[i] &= ~TRAPSET;
			if (t = trapcom[i])
			{
				int     savxit = exitval;

				execexp(t, 0);
				exitval = savxit;
				exitset();
			}
		}
	}
}
