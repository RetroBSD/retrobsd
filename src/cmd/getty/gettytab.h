/*
 * Getty description definitions.
 *
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

struct	gettystrs {
	char	*field;		/* name to lookup in gettytab */
	char	*defalt;	/* value we find by looking in defaults */
	char	*value;		/* value that we find there */
};

struct	gettynums {
	char	*field;		/* name to lookup */
	long	defalt;		/* number we find in defaults */
	long	value;		/* number we find there */
	int	set;		/* we actually got this one */
};

struct gettyflags {
	char	*field;		/* name to lookup */
	char	invrt;		/* name existing in gettytab --> false */
	char	defalt;		/* true/false in defaults */
	char	value;		/* true/false flag */
	char	set;		/* we found it */
};

/*
 * String values.
 */
#define	NX	gettystrs[0].value
#define	CL	gettystrs[1].value
#define IM	gettystrs[2].value
#define	LM	gettystrs[3].value
#define	ER	gettystrs[4].value
#define	KL	gettystrs[5].value
#define	ET	gettystrs[6].value
#define	PC	gettystrs[7].value
#define	TT	gettystrs[8].value
#define	EV	gettystrs[9].value
#define	LO	gettystrs[10].value
#define HN	gettystrs[11].value
#define HE	gettystrs[12].value
#define IN	gettystrs[13].value
#define QU	gettystrs[14].value
#define XN	gettystrs[15].value
#define XF	gettystrs[16].value
#define BK	gettystrs[17].value
#define SU	gettystrs[18].value
#define DS	gettystrs[19].value
#define RP	gettystrs[20].value
#define FL	gettystrs[21].value
#define WE	gettystrs[22].value
#define LN	gettystrs[23].value

/*
 * Numeric definitions.
 */
#define	IS	gettynums[0].value
#define	OS	gettynums[1].value
#define	SP	gettynums[2].value
#define	TO	gettynums[3].value
#define	F0	gettynums[4].value
#define	F0set	gettynums[4].set
#define	F1	gettynums[5].value
#define	F1set	gettynums[5].set
#define	F2	gettynums[6].value
#define	F2set	gettynums[6].set
#define	PF	gettynums[7].value

/*
 * Boolean values.
 */
#define	HT	gettyflags[0].value
#define	NL	gettyflags[1].value
#define	EP	gettyflags[2].value
#define	EPset	gettyflags[2].set
#define	OP	gettyflags[3].value
#define	OPset	gettyflags[2].set
#define	AP	gettyflags[4].value
#define	APset	gettyflags[2].set
#define	EC	gettyflags[5].value
#define	CO	gettyflags[6].value
#define	CB	gettyflags[7].value
#define	CK	gettyflags[8].value
#define	CE	gettyflags[9].value
#define	PE	gettyflags[10].value
#define	RW	gettyflags[11].value
#define	XC	gettyflags[12].value
#define	IG	gettyflags[13].value
#define	PS	gettyflags[14].value
#define	HC	gettyflags[15].value
#define UB	gettyflags[16].value
#define AB	gettyflags[17].value
#define DX	gettyflags[18].value
#define	HF	gettyflags[19].value

int	getent();
long	getnum();
int	getflag();
char	*getstr();

long	setflags();

extern	struct gettyflags gettyflags[];
extern	struct gettynums gettynums[];
extern	struct gettystrs gettystrs[];
extern	int hopcount;

void gettable(char *name, char *buf, char *area);
void gendefaults(void);
void setdefaults(void);
int speed(long val);
void setchars(void);
char *portselector(void);
void edithost(char *pat);
void makeenv(char *env[]);
void get_date(char *datebuffer);
