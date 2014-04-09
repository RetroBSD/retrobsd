/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)mon.c	5.5 (GTE) 3/23/92";
#endif LIBC_SCCS and not lint

#define ARCDENSITY	1	/* density of routines per 100 bytes */
#define MINARCS		50	/* minimum number of counters */
#define	HISTFRACTION	8	/* fraction of text space for histograms */


struct phdr {		/* mon.out header */
	int	*lpc;		/* low pc of histogramed text space */
	int	*hpc;		/* high pc */
	int	ncnt;		/* number of functions counted */
};

struct cnt {		/* function entry count structure */
	int	(*pc)();	/* address of profiled function */
	long	ncall;		/* number of time function called */
};

static struct cnt *countbase;	/* next free cnt struct */
static struct cnt *countend;	/* first address past cnt structs */

static short	*s_sbuf;	/* start of histogram buffer */
static unsigned	s_bufsize;	/* size of histogram buffer (in chars) */
static char	*s_lowpc;	/* low pc for histgram recording */
static unsigned	s_scale;	/* histogram scale */

#define	PERROR(s)	write(2, s, sizeof(s)-1)

monstartup(lowpc, highpc)
	char *lowpc;
	char *highpc;
{
	unsigned int cntsize, monsize;
	char *buffer;
	extern char *sbrk();
	extern char *minbrk;

	cntsize = (unsigned)(highpc - lowpc) * ARCDENSITY / 100;
	if (cntsize < MINARCS)
		cntsize = MINARCS;
	monsize = (unsigned)(highpc - lowpc + HISTFRACTION - 1) / HISTFRACTION
		+ sizeof(struct phdr) + cntsize * sizeof(struct cnt);
	monsize = (monsize + 1) & ~1;
	buffer = sbrk(monsize);
	if (buffer == (char *)-1) {
		PERROR("monstartup: no space for monitor buffer(s)\n");
		return;
	}
	minbrk = sbrk(0);
	monitor(lowpc, highpc, buffer, monsize>>1, cntsize);
}

monitor(lowpc, highpc, buf, bufsize, cntsize)
	char *lowpc, *highpc;
	char *buf;		/* really (short *) but easier this way */
	unsigned bufsize, cntsize;
{
	register unsigned o;
	register struct phdr *php;
	static char *sbuf;	/* saved base of profiling buffer */
	static unsigned ssize;	/* saved buffer size */

	if (lowpc == 0) {
		moncontrol(0);
		o = creat("mon.out", 0666);
		write(o, sbuf, ssize);
		close(o);
		return;
	}
	bufsize *= sizeof(short);
	if (bufsize < sizeof(struct phdr)+sizeof(struct cnt)+sizeof(short)) {
		PERROR("monitor: buffer too small");
		return;
	}
	sbuf = buf;
	ssize = bufsize;

	countbase = (struct cnt *)(buf + sizeof(struct phdr));
	o = sizeof(struct phdr) + cntsize * sizeof(struct cnt);
	if (o > bufsize) {
		cntsize = (bufsize - sizeof(struct phdr))/sizeof(struct cnt);
		o = sizeof(struct phdr) + cntsize * sizeof(struct cnt);
	}
	countend = (struct cnt *)(buf + o);

	php = (struct phdr *)buf;
	php->lpc = (int *)lowpc;
	php->hpc = (int *)highpc;
	php->ncnt = cntsize;

	s_sbuf = (short *)countend;
	s_bufsize = bufsize - o;
	s_lowpc = lowpc;
	o = highpc - lowpc;
	if(s_bufsize < o)
		o = ((long)s_bufsize << 16) / o;
	else
		o = 0xffff;
	s_scale = o;
	moncontrol(1);
}

/*
 * Control profiling
 */
moncontrol(mode)
	int mode;
{
	if (mode) {
		/* start */
		profil(s_sbuf, s_bufsize, s_lowpc, s_scale);
	} else {
		/* stop */
		profil((char *)0, 0, 0, 0);
	}
}
