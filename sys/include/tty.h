#ifndef _TTY_H
#define _TTY_H
/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifdef KERNEL
#include "types.h"
#include "ttychars.h"
#include "ttydev.h"
#include "ioctl.h"
#else
#include <sys/types.h>
#include <sys/ttychars.h>
#include <sys/ttydev.h>
#include <sys/ioctl.h>
#endif

/*
 * A clist structure is the head of a linked list queue
 * of characters.  The characters are stored in blocks
 * containing a link and CBSIZE (param.h) characters.
 * The routines in tty_subr.c manipulate these structures.
 */
struct clist {
	int	c_cc;		/* character count */
	char	*c_cf;		/* pointer to first char */
	char	*c_cl;		/* pointer to last char */
};

/*
 * Per-tty structure.
 *
 * Should be split in two, into device and tty drivers.
 * Glue could be masks of what to echo and circular buffer
 * (low, high, timeout).
 */
struct tty {
	union {
		struct {
			struct	clist T_rawq;
			struct	clist T_canq;
		} t_t;
#define	t_rawq	t_nu.t_t.T_rawq		/* raw characters or partial line */
#define	t_canq	t_nu.t_t.T_canq		/* raw characters or partial line */
		struct {
			struct	buf *T_bufp;
			char	*T_cp;
			int	T_inbuf;
			int	T_rec;
		} t_n;
#define	t_bufp	t_nu.t_n.T_bufp		/* buffer allocated to protocol */
#define	t_cp	t_nu.t_n.T_cp		/* pointer into the ripped off buffer */
#define	t_inbuf	t_nu.t_n.T_inbuf	/* number chars in the buffer */
#define	t_rec	t_nu.t_n.T_rec		/* have a complete record */
	} t_nu;
	struct	clist t_outq;		/* device */
	void	(*t_oproc) (struct tty*);
	struct	proc *t_rsel;		/* tty */
	struct	proc *t_wsel;
	caddr_t	T_LINEP;		/* ### */
	caddr_t	t_addr;			/* ??? */
	dev_t	t_dev;			/* device */
	long	t_flags;		/* some of both */
	long	t_state;		/* some of both */
	int	t_pgrp;			/* tty */
	int	t_delct;		/* tty */
	int	t_col;			/* tty */
	int	t_ispeed, t_ospeed;	/* device */
	int	t_rocount, t_rocol;	/* tty */
	struct	ttychars t_chars;	/* tty */
	struct	winsize t_winsize;	/* window size */
/* be careful of tchars & co. */
#define	t_erase		t_chars.tc_erase
#define	t_kill		t_chars.tc_kill
#define	t_intrc		t_chars.tc_intrc
#define	t_quitc		t_chars.tc_quitc
#define	t_startc	t_chars.tc_startc
#define	t_stopc		t_chars.tc_stopc
#define	t_eofc		t_chars.tc_eofc
#define	t_brkc		t_chars.tc_brkc
#define	t_suspc		t_chars.tc_suspc
#define	t_dsuspc	t_chars.tc_dsuspc
#define	t_rprntc	t_chars.tc_rprntc
#define	t_flushc	t_chars.tc_flushc
#define	t_werasc	t_chars.tc_werasc
#define	t_lnextc	t_chars.tc_lnextc
};

#define	TTIPRI	28
#define	TTOPRI	29

/* limits */
#define	NSPEEDS	29
#define	TTMASK	15
#define	OBUFSIZ	100

#ifdef KERNEL

extern const int tthiwat[NSPEEDS], ttlowat[NSPEEDS];
extern int q_to_b(register struct clist *q, char *cp, int cc);

#define	TTHIWAT(tp)	tthiwat[(tp)->t_ospeed&TTMASK]
#define	TTLOWAT(tp)	ttlowat[(tp)->t_ospeed&TTMASK]

extern int nldisp;		/* number of line disciplines */

/*
 * Set t_chars to default values.
 */
void ttychars (struct tty *tp);

/*
 * Clean terminal on last close.
 */
void ttyclose (struct tty *tp);

/*
 * Wakeup processes waiting on output flow control.
 */
void ttyowake (struct tty *tp);

/*
 * Get a symbol from a character list.
 */
int getc (struct clist *p);

/*
 * Get the pointer to the next character in the list.
 */
char *nextc (struct clist *p, char *cp);

/*
 * Put a symbol to a character list.
 */
int putc (int c, struct clist *p);

/*
 * Remove the last character in the list and return it.
 */
int unputc (struct clist *p);

/*
 * Put the chars in the from que on the end of the to que.
 */
void catq (struct clist *from, struct clist *to);

/*
 * Copy buffer to clist.
 */
int b_to_q (char *cp, int nbytes, struct clist *q);

/*
 * Common code for tty ioctls.
 */
int ttioctl (struct tty *tp, u_int com, caddr_t data, int flag);

/*
 * Start output on the typewriter.
 */
void ttstart (struct tty *tp);

void ttwakeup (struct tty *tp);

/*
 * Place a character on raw TTY input queue,
 */
void ttyinput (int c, struct tty *tp);

/*
 * Put character on TTY output queue.
 */
int ttyoutput (int c, struct tty *tp);

/*
 * Initial open of tty, or (re)entry to line discipline.
 */
int ttyopen (dev_t dev, struct tty *tp);

/*
 * Close a line discipline.
 */
int ttylclose (struct tty *tp, int flag);

/*
 * Check the output queue for space.
 */
int ttycheckoutq (struct tty *tp, int wait);

/*
 * Called from device's read routine after it has
 * calculated the tty-structure given as argument.
 */
struct uio;
int ttread (struct tty *tp, struct uio *uio, int flag);
int ttwrite (struct tty *tp, struct uio *uio, int flag);

/*
 * Handle modem control transition on a tty.
 */
int ttymodem (struct tty *tp, int flag);

/*
 * Check that input or output is possible on a terminal.
 */
int ttyselect (struct tty *tp, int rw);

/*
 * Flush all TTY queues.
 */
void ttyflush (struct tty *tp, int rw);

#endif /* KERNEL */

/* internal state bits */
#define	TS_TIMEOUT	0x000001L	/* delay timeout in progress */
#define	TS_WOPEN	0x000002L	/* waiting for open to complete */
#define	TS_ISOPEN	0x000004L	/* device is open */
#define	TS_FLUSH	0x000008L	/* outq has been flushed during DMA */
#define	TS_CARR_ON	0x000010L	/* software copy of carrier-present */
#define	TS_BUSY		0x000020L	/* output in progress */
#define	TS_ASLEEP	0x000040L	/* wakeup when output done */
#define	TS_XCLUDE	0x000080L	/* exclusive-use flag against open */
#define	TS_TTSTOP	0x000100L	/* output stopped by ctl-s */
#define	TS_HUPCLS	0x000200L	/* hang up upon last close */
#define	TS_TBLOCK	0x000400L	/* tandem queue blocked */
#define	TS_RCOLL	0x000800L	/* collision in read select */
#define	TS_WCOLL	0x001000L	/* collision in write select */
#define	TS_ASYNC	0x004000L	/* tty in async i/o mode */
/* state for intra-line fancy editing work */
#define	TS_ERASE	0x040000L	/* within a \.../ for PRTRUB */
#define	TS_LNCH		0x080000L	/* next character is literal */
#define	TS_TYPEN	0x100000L	/* retyping suspended input (PENDIN) */
#define	TS_CNTTB	0x200000L	/* counting tab width; leave FLUSHO alone */

#define	TS_LOCAL	(TS_ERASE|TS_LNCH|TS_TYPEN|TS_CNTTB)

/* define partab character types */
#define	ORDINARY	0
#define	CONTROL		1
#define	BACKSPACE	2
#define	NEWLINE		3
#define	TAB		4
#define	VTAB		5
#define	RETURN		6
#endif
