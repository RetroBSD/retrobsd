/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 */
#include "defs.h"
#include "sym.h"

/* ======== storage allocation for functions ======== */

char *
getstor(asize)
	int asize;
{
	if (fndef)
		return alloc(asize);
	else
		return getstak(asize);
}

/* ========	command line decoding	========*/

struct trenod *
makefork(flgs, i)
	int	flgs;
	struct trenod *i;
{
	register struct forknod *t;

	t = (struct forknod *)getstor(sizeof(struct forknod));
	t->forktyp = flgs|TFORK;
	t->forktre = i;
	t->forkio = NIL;
	return((struct trenod *)t);
}

static int
prsym(sym)
{
	if (sym & SYMFLG)
	{
		register struct sysnod *sp = reserved;

		while (sp->sysval && sp->sysval != sym)
			sp++;
		prs(sp->sysnam);
	}
	else if (sym == EOFSYM)
		prs(endoffile);
	else
	{
		if (sym & SYMREP)
			prc(sym);
		if (sym == NL)
			prs("newline or ;");
		else
			prc(sym);
	}
}

static int
synbad()
{
	prp();
	prs(synmsg);
	if ((flags & ttyflg) == 0)
	{
		prs(atline);
		prn(standin->flin);
	}
	prs(colon);
	prc(LQ);
	if (wdval)
		prsym(wdval);
	else
		prs_cntl(wdarg->argval);
	prc(RQ);
	prs(unexpected);
	newline();
	exitsh(SYNBAD);
}

static struct trenod *
makelist(type, i, r)
	int	type;
	struct trenod *i, *r;
{
	register struct lstnod *t;

	if (i == NIL || r == NIL)
		synbad();
	else
	{
		t = (struct lstnod *)getstor(sizeof(struct lstnod));
		t->lsttyp = type;
		t->lstlef = i;
		t->lstrit = r;
	}
	return((struct trenod *)t);
}

static int
skipnl()
{
	while ((reserv++, word() == NL))
		chkpr();
	return(wdval);
}

static int
chksym(sym)
{
	register int	x = sym & wdval;

	if (((x & SYMFLG) ? x : sym) != wdval)
		synbad();
}

static struct regnod *
syncase(esym)
register int	esym;
{
	skipnl();
	if (wdval == esym)
		return(NIL);
	else
	{
		register struct regnod *r = (struct regnod *)getstor(sizeof(struct regnod));
		register struct argnod *argp;

		r->regptr = NIL;
		for (;;)
		{
			if (fndef)
			{
				argp= wdarg;
				wdarg = (struct argnod *)alloc(length(argp->argval) + BYTESPERWORD);
				movstr(argp->argval, wdarg->argval);
			}

			wdarg->argnxt = r->regptr;
			r->regptr = wdarg;
			if (wdval || (word() != ')' && wdval != '|'))
				synbad();
			if (wdval == '|')
				word();
			else
				break;
		}
		r->regcom = cmd(0, NLFLG | MTFLG);
		if (wdval == ECSYM)
			r->regnxt = syncase(esym);
		else
		{
			chksym(esym);
			r->regnxt = NIL;
		}
		return(r);
	}
}

static int
chkword()
{
	if (word())
		synbad();
}

static struct ionod *
inout(lastio)
	struct ionod *lastio;
{
	register int	iof;
	register struct ionod *iop;
	register char	c;

	iof = wdnum;
	switch (wdval)
	{
	case DOCSYM:	/*	<<	*/
		iof |= IODOC;
		break;

	case APPSYM:	/*	>>	*/
	case '>':
		if (wdnum == 0)
			iof |= 1;
		iof |= IOPUT;
		if (wdval == APPSYM)
		{
			iof |= IOAPP;
			break;
		}

	case '<':
		if ((c = nextc(0)) == '&')
			iof |= IOMOV;
		else if (c == '>')
			iof |= IORDW;
		else
			peekc = c | MARK;
		break;

	default:
		return(lastio);
	}

	chkword();
	iop = (struct ionod *)getstor(sizeof(struct ionod));

	if (fndef)
		iop->ioname = make(wdarg->argval);
	else
		iop->ioname = wdarg->argval;

	iop->iolink = NIL;
	iop->iofile = iof;
	if (iof & IODOC)
	{
		iop->iolst = iopend;
		iopend = iop;
	}
	word();
	iop->ionxt = inout(lastio);
	return(iop);
}

/*
 * item
 *
 *	( cmd ) [ < in  ] [ > out ]
 *	word word* [ < in ] [ > out ]
 *	if ... then ... else ... fi
 *	for ... while ... do ... done
 *	case ... in ... esac
 *	begin ... end
 */
static struct trenod *
item(flag)
	BOOL	flag;
{
	register struct trenod *r;
	register struct ionod *io;

	if (flag)
		io = inout((struct ionod *)NIL);
	else
		io = NIL;
	switch (wdval)
	{
	case CASYM:
		{
			register struct swnod *t;

			t = (struct swnod *)getstor(sizeof(struct swnod));
			r = (struct trenod *)t;

			chkword();
			if (fndef)
				t->swarg = make(wdarg->argval);
			else
				t->swarg = wdarg->argval;
			skipnl();
			chksym(INSYM | BRSYM);
			t->swlst = syncase(wdval == INSYM ? ESSYM : KTSYM);
			t->swtyp = TSW;
			break;
		}

	case IFSYM:
		{
			register int	w;
			register struct ifnod *t;

			t = (struct ifnod *)getstor(sizeof(struct ifnod));
			r = (struct trenod *)t;

			t->iftyp = TIF;
			t->iftre = cmd(THSYM, NLFLG);
			t->thtre = cmd(ELSYM | FISYM | EFSYM, NLFLG);
			t->eltre = ((w = wdval) == ELSYM) ? cmd(FISYM, NLFLG)        :
				    (w          == EFSYM) ? (wdval = IFSYM, item(0)) :
				    (struct trenod *)NIL;
			if (w == EFSYM)
				return(r);
			break;
		}

	case FORSYM:
		{
			register struct fornod *t;

			t = (struct fornod *)getstor(sizeof(struct fornod));
			r = (struct trenod *)t;

			t->fortyp = TFOR;
			t->forlst = NIL;
			chkword();
			if (fndef)
				t->fornam = make(wdarg->argval);
			else
				t->fornam = wdarg->argval;
			if (skipnl() == INSYM)
			{
				chkword();

				nohash++;
				t->forlst = (struct comnod *)item(0);
				nohash--;

				if (wdval != NL && wdval != ';')
					synbad();
				if (wdval == NL)
					chkpr();
				skipnl();
			}
			chksym(DOSYM | BRSYM);
			t->fortre = cmd(wdval == DOSYM ? ODSYM : KTSYM, NLFLG);
			break;
		}

	case WHSYM:
	case UNSYM:
		{
			register struct whnod *t;

			t = (struct whnod *)getstor(sizeof(struct whnod));
			r = (struct trenod *)t;

			t->whtyp = (wdval == WHSYM ? TWH : TUN);
			t->whtre = cmd(DOSYM, NLFLG);
			t->dotre = cmd(ODSYM, NLFLG);
			break;
		}

	case BRSYM:
		r = cmd(KTSYM, NLFLG);
		break;

	case '(':
		{
			register struct parnod *p;

			p = (struct parnod *)getstor(sizeof(struct parnod));
			p->partre = cmd(')', NLFLG);
			p->partyp = TPAR;
			r = makefork(0, p);
			break;
		}

	default:
		if (io == NIL)
			return(NIL);

	case 0:
		{
			register struct comnod *t;
			register struct argnod *argp;
			register struct argnod **argtail;
			register struct argnod **argset = (struct argnod **)NIL;
			int	keywd = 1;
			char	*com;

			if ((wdval != NL) && ((peekn = skipc()) == '('))
			{
				struct fndnod *f;
				struct ionod  *saveio;

				saveio = iotemp;
				peekn = 0;
				if (skipc() != ')')
					synbad();

				f = (struct fndnod *)getstor(sizeof(struct fndnod));
				r = (struct trenod *)f;

				f->fndtyp = TFND;
				if (fndef)
					f->fndnam = make(wdarg->argval);
				else
					f->fndnam = wdarg->argval;
				reserv++;
				fndef++;
				skipnl();
				f->fndval = (struct trenod *)item(0);
				fndef--;

				if (iotemp != saveio)
				{
					struct ionod 	*ioptr = iotemp;

					while (ioptr->iolst != saveio)
						ioptr = ioptr->iolst;

					ioptr->iolst = fiotemp;
					fiotemp = iotemp;
					iotemp = saveio;
				}
				return(r);
			}
			else
			{
				t = (struct comnod *)getstor(sizeof(struct comnod));
				r = (struct trenod *)t;

				t->comio = io; /*initial io chain*/
				argtail = &(t->comarg);

				while (wdval == 0)
				{
					if (fndef)
					{
						argp = wdarg;
						wdarg = (struct argnod *)alloc(length(argp->argval) + BYTESPERWORD);
						movstr(argp->argval, wdarg->argval);
					}

					argp = wdarg;
					if (wdset && keywd)
					{
						argp->argnxt = (struct argnod *)argset;
						argset = (struct argnod **)argp;
					}
					else
					{
						*argtail = argp;
						argtail = &(argp->argnxt);
						keywd = flags & keyflg;
					}
					word();
					if (flag)
						t->comio = inout(t->comio);
				}

				t->comtyp = TCOM;
				t->comset = (struct argnod *)argset;
				*argtail = NIL;

				if (nohash == 0 && (fndef == 0 || (flags & hashflg)))
				{
					if (t->comarg)
					{
						com = t->comarg->argval;
						if (*com && *com != DOLLAR)
							pathlook(com, 0, t->comset);
					}
				}

				return(r);
			}
		}

	}
	reserv++;
	word();
	if (io = inout(io))
	{
		r = makefork(0,r);
		r->treio = io;
	}
	return(r);
}

/*
 * term
 *	item
 *	item |^ term
 */
static struct trenod *
term(flg)
{
	register struct trenod *t;

	reserv++;
	if (flg & NLFLG)
		skipnl();
	else
		word();
	if ((t = item(TRUE)) && (wdval == '^' || wdval == '|'))
	{
		struct trenod	*left;
		struct trenod	*right;

		left = makefork(FPOU, t);
		right = makefork(FPIN | FPCL, term(NLFLG));
		return(makefork(0, makelist(TFIL, left, right)));
	}
	else
		return(t);
}

/*
 * list
 *	term
 *	list && term
 *	list || term
 */
static struct trenod *
list(flg)
{
	register struct trenod *r;
	register int		b;

	r = term(flg);
	while (r && ((b = (wdval == ANDFSYM)) || wdval == ORFSYM))
		r = makelist((b ? TAND : TORF), r, term(NLFLG));
	return(r);
}

/*
 * cmd
 *	empty
 *	list
 *	list & [ cmd ]
 *	list [ ; cmd ]
 */
struct trenod *
cmd(sym, flg)
	register int	sym;
	int		flg;
{
	register struct trenod *i, *e;

	i = list(flg);
	if (wdval == NL)
	{
		if (flg & NLFLG)
		{
			wdval = ';';
			chkpr();
		}
	}
	else if (i == NIL && (flg & MTFLG) == 0)
		synbad();

	switch (wdval)
	{
	case '&':
		if (i)
			i = makefork(FINT | FPRS | FAMP, i);
		else
			synbad();

	case ';':
		if (e = cmd(sym, flg | MTFLG))
			i = makelist(TLST, i, e);
		else if (i == NIL)
			synbad();
		break;

	case EOFSYM:
		if (sym == NL)
			break;

	default:
		if (sym)
			chksym(sym);
	}
	return(i);
}
