/*
 * Print system stuff
 *
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/param.h>
#define	KERNEL
#include <sys/file.h>
#include <sys/user.h>
#undef	KERNEL
#include <sys/proc.h>
#include <sys/inode.h>
#include <sys/map.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/vm.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>

char	*fcore	= "/dev/kmem";
char	*fmem	= "/dev/mem";
int	fc, fm;

struct nlist nl[] = {
#define	SINODE	0
	{ "_inode" },
#define	SPROC	1
	{ "_proc" },
#define	SKL	2
	{ "_cnttys" },
#define	SFIL	3
	{ "_file" },
#define	SNSWAP	4
	{ "_nswap" },
#define	SWAPMAP	5
	{ "_swapmap" },
#define	SNPROC	6
	{ "_nproc" },
	{ "" }
};

int	inof;
int	prcf;
int	ttyf;
int	usrf;
long	ubase;
int	filf;
int	swpf;
int	totflg;
int	allflg;
int	kflg;

main(argc, argv)
char **argv;
{
	register char *argp;
	int allflags;

	argc--, argv++;
	while (argc > 0 && **argv == '-') {
		argp = *argv++;
		argp++;
		argc--;
		while (*argp++)
		switch (argp[-1]) {

		case 'T':
			totflg++;
			break;

		case 'a':
			allflg++;
			break;

		case 'i':
			inof++;
			break;

		case 'k':
			kflg++;
			fcore = fmem = "/core";
			break;

		case 'p':
			prcf++;
			break;

		case 't':
			ttyf++;
			break;

		case 'u':
			if (argc == 0)
				break;
			argc--;
			usrf++;
			sscanf( *argv++, "%lx", &ubase);
			break;

		case 'f':
			filf++;
			break;
		case 's':
			swpf++;
			break;
		default:
			usage();
			exit(1);
		}
	}
	if (argc>0) {
		fcore = fmem = argv[0];
		kflg++;
	}
	if ((fc = open(fcore, 0)) < 0) {
		printf("Can't find %s\n", fcore);
		exit(1);
	}
	if ((fm = open(fmem, 0)) < 0) {
		printf("Can't find %s\n", fmem);
		exit(1);
	}
	knlist(nl);

	if (nl[0].n_value == 0) {
		printf("no namelist, n_type: %d n_value: %x n_name: %s\n", nl[0].n_type, nl[0].n_value, nl[0].n_name);
		exit(1);
	}
	if (! (filf | totflg | inof | prcf | ttyf | usrf | swpf))
		filf++;

	if (filf||totflg)
		dofile();
	if (inof||totflg)
		doinode();
	if (prcf||totflg)
		doproc();
	if (ttyf)
		dotty();
	if (usrf)
		dousr();
	if (swpf||totflg)
		doswap();
}

usage()
{
	printf("usage: pstat -[aikptfsT] [-u [ubase]] [core]\n");
}

doinode()
{
	register struct inode *ip;
	struct inode *xinode;
	register int nin;
	u_int ainode;

	nin = 0;
	xinode = (struct inode *)calloc(NINODE, sizeof (struct inode));
	ainode = nl[SINODE].n_value;
	if (xinode == NULL) {
		fprintf(stderr, "can't allocate memory for inode table\n");
		return;
	}
	lseek(fc, (off_t)ainode, 0);
	read(fc, xinode, NINODE * sizeof(struct inode));
	for (ip = xinode; ip < &xinode[NINODE]; ip++)
		if (ip->i_count)
			nin++;
	if (totflg) {
		printf("%3d/%3d inodes\n", nin, NINODE);
		return;
	}
	printf("%d/%d active inodes\n", nin, NINODE);
        printf("   LOC       FLAGS      CNT  DEVICE  RDC WRC  INO   MODE  NLK  UID  SIZE/DEV FS\n");
	for (ip = xinode; ip < &xinode[NINODE]; ip++) {
		if (ip->i_count == 0)
			continue;
		printf("%08x ", ainode + (ip - xinode)*sizeof (*ip));
		putf((long)ip->i_flag&ILOCKED, 'L');
		putf((long)ip->i_flag&IUPD, 'U');
		putf((long)ip->i_flag&IACC, 'A');
		putf((long)ip->i_flag&IMOUNT, 'M');
		putf((long)ip->i_flag&IWANT, 'W');
		putf((long)ip->i_flag&ITEXT, 'T');
		putf((long)ip->i_flag&ICHG, 'C');
		putf((long)ip->i_flag&ISHLOCK, 'S');
		putf((long)ip->i_flag&IEXLOCK, 'E');
		putf((long)ip->i_flag&ILWAIT, 'Z');
		putf((long)ip->i_flag&IPIPE, 'P');
		putf((long)ip->i_flag&IMOD, 'm');
		putf((long)ip->i_flag&IRENAME, 'r');
		putf((long)ip->i_flag&IXMOD, 'x');
		printf("%4d", ip->i_count);
		printf("%4d,%3d", major(ip->i_dev), minor(ip->i_dev));
		printf("%4d", ip->i_flag&IPIPE ? 0 : ip->i_shlockc);
		printf("%4d", ip->i_flag&IPIPE ? 0 : ip->i_exlockc);
		printf("%6u ", ip->i_number);
		printf("%7.1o", ip->i_mode);
		printf("%4d", ip->i_nlink);
		printf("%5u", ip->i_uid);
		if ((ip->i_mode&IFMT)==IFBLK || (ip->i_mode&IFMT)==IFCHR)
			printf("%6d,%3d", major(ip->i_rdev), minor(ip->i_rdev));
		else
			printf("%10ld", ip->i_size);
		printf(" %08x", ip->i_fs);
		printf("\n");
	}
	free(xinode);
}

u_int
getuint(loc)
	off_t loc;
{
	u_int word;

	lseek(fc, loc, 0);
	read(fc, &word, sizeof (word));

	return (word);
}

putf(v, n)
	long	v;
	char	n;
{
	if (v)
		printf("%c", n);
	else
		printf(" ");
}

doproc()
{
	struct proc *xproc;
	u_int nproc, aproc;
	register struct proc *pp;
	register loc, np;

	nproc = getuint((off_t)nl[SNPROC].n_value);
	xproc = (struct proc *)calloc(nproc, sizeof (struct proc));
	aproc = nl[SPROC].n_value;
	if (nproc < 0 || nproc > 10000) {
		fprintf(stderr, "number of procs is preposterous (%d)\n",
			nproc);
		return;
	}
	if (xproc == NULL) {
		fprintf(stderr, "can't allocate memory for proc table\n");
		return;
	}
	lseek(fc, (off_t)aproc, 0);
	read(fc, xproc, nproc * sizeof (struct proc));
	np = 0;
	for (pp=xproc; pp < &xproc[nproc]; pp++)
		if (pp->p_stat)
			np++;
	if (totflg) {
		printf("%3d/%3d processes\n", np, nproc);
		return;
	}
	printf("%d/%d processes\n", np, nproc);
        printf("   LOC    S       F PRI      SIG   UID SLP TIM  CPU  NI   PGRP    PID   PPID     ADDR    SADDR    DADDR     SIZE   WCHAN    LINK     SIGM\n");
	for (pp=xproc; pp<&xproc[nproc]; pp++) {
		if (pp->p_stat==0 && allflg==0)
			continue;
		printf("%08x", aproc + (pp - xproc)*sizeof (*pp));
		printf(" %2d", pp->p_stat);
		printf(" %7.1x", pp->p_flag);
		printf(" %3d", pp->p_pri);
		printf(" %8.1lx", pp->p_sig);
		printf(" %5u", pp->p_uid);
		printf(" %3d", pp->p_slptime);
		printf(" %3d", pp->p_time);
		printf(" %4d", pp->p_cpu&0377);
		printf(" %3d", pp->p_nice);
		printf(" %6d", pp->p_pgrp);
		printf(" %6d", pp->p_pid);
		printf(" %6d", pp->p_ppid);
		printf(" %8x", pp->p_addr);
		printf(" %8x", pp->p_saddr);
		printf(" %8x", pp->p_daddr);
		printf(" %6x", pp->p_dsize+pp->p_ssize);
		printf(" %8x", pp->p_wchan);
		printf(" %8x", pp->p_link);
		printf(" %8.1lx", pp->p_sigmask);
		printf("\n");
	}
	free(xproc);
}

static int ttyspace = 64;
static struct tty *tty;

dotty()
{
	if ((tty = (struct tty *)malloc(ttyspace * sizeof(*tty))) == 0) {
		printf("pstat: out of memory\n");
		return;
	}
	dottytype("cn", SKL);
}

dottytype(name, type)
char *name;
{
	register struct tty *tp;

	printf("%s line\n", name);
	lseek(fc, (long)nl[type].n_value, 0);
	read(fc, tty, sizeof(struct tty));
	printf(" # RAW CAN OUT         MODE     ADDR  DEL  COL     STATE       PGRP\n");
	ttyprt(tty, 0);
}

ttyprt(atp, line)
struct tty *atp;
{
	register struct tty *tp;

	printf("%2d", line);
	tp = atp;

	printf("%4d%4d", tp->t_rawq.c_cc, tp->t_canq.c_cc);
	printf("%4d %12.1lo %8x %4d %4d ", tp->t_outq.c_cc, tp->t_flags,
		tp->t_addr, tp->t_delct, tp->t_col);
	putf(tp->t_state&TS_TIMEOUT, 'T');
	putf(tp->t_state&TS_WOPEN, 'W');
	putf(tp->t_state&TS_ISOPEN, 'O');
	putf(tp->t_state&TS_FLUSH, 'F');
	putf(tp->t_state&TS_CARR_ON, 'C');
	putf(tp->t_state&TS_BUSY, 'B');
	putf(tp->t_state&TS_ASLEEP, 'A');
	putf(tp->t_state&TS_XCLUDE, 'X');
	putf(tp->t_state&TS_TTSTOP, 'S');
	putf(tp->t_state&TS_HUPCLS, 'H');
	putf(tp->t_state&TS_TBLOCK, 'b');
	putf(tp->t_state&TS_RCOLL, 'r');
	putf(tp->t_state&TS_WCOLL, 'w');
	putf(tp->t_state&TS_ASYNC, 'a');
	printf("%6d\n", tp->t_pgrp);
}

dousr()
{
	struct user U;
	long	*ip;
	register i, j;

	lseek(fm, ubase, 0);
	read(fm, &U, sizeof(U));
	printf("procp\t%p\n", U.u_procp);
	printf("frame\t%p\n", U.u_frame);
	printf("comm\t%s\n", U.u_comm);
	printf("arg\t%p %p %p %p %p %p\n", U.u_arg[0], U.u_arg[1],
		U.u_arg[2], U.u_arg[3], U.u_arg[4], U.u_arg[5]);
	printf("qsave\t");
	for (i = 0; i < sizeof (label_t) / sizeof (int); i++)
		printf("%p ", U.u_qsave.val[i]);
	printf("\n");
	printf("rval\t%p\n", U.u_rval);
	printf("error\t%d\n", U.u_error);
	printf("uids\t%d,%d,%d,%d,%d\n", U.u_uid, U.u_svuid, U.u_ruid,
		U.u_svgid, U.u_rgid);
	printf("groups");
	for (i = 0; (i < NGROUPS) && (U.u_groups[i] != NOGROUP); i++) {
		if (i%8 == 0) printf("\t");
		printf("%u ", U.u_groups[i]);
		if (i%8 == 7) printf("\n");
	}
	if (i%8) printf("\n");
	printf("tsize\t%.1x\n", U.u_tsize);
	printf("dsize\t%.1x\n", U.u_dsize);
	printf("ssize\t%.1x\n", U.u_ssize);
	printf("ssave\t");
	for (i = 0; i < sizeof (label_t) / sizeof (int); i++)
		printf("%.1x ", U.u_ssave.val[i]);
	printf("\n");
	printf("rsave\t");
	for	(i = 0; i < sizeof (label_t) / sizeof (int); i++)
		printf("%.1x ", U.u_rsave.val[i]);
	printf("\n");
	printf("signal");
	for (i = 0; i < NSIG; i++) {
		if (i%8 == 0) printf("\t");
		printf("%.1x ", U.u_signal[i]);
		if (i%8 == 7) printf("\n");
	}
	if (i%8) printf("\n");
	printf("sigmask");
	for (i = 0; i < NSIG; i++) {
		if (i%8 == 0) printf("\t");
		printf("%.1lx ", U.u_sigmask[i]);
		if (i%8 == 7) printf("\n");
	}
	if (i%8) printf("\n");
	printf("sigonstack\t%.1lx\n", U.u_sigonstack);
	printf("sigintr\t%.1lx\n", U.u_sigintr);
	printf("oldmask\t%.1lx\n", U.u_oldmask);
	printf("code\t%08x\n", U.u_code);
	printf("psflags\t%d\n", U.u_psflags);
	printf("ss_base\t%.1x ss_size %.1x ss_flags %.1x\n",
		U.u_sigstk.ss_base, U.u_sigstk.ss_size, U.u_sigstk.ss_flags);
	printf("ofile");
	for	(i = 0; i < NOFILE; i++)
		{
		if	(i%8 == 0) printf("\t");
		printf("%.1x ", U.u_ofile[i]);
		if	(i%8 == 7) printf("\n");
		}
	if	(i%8) printf("\n");
	printf("pofile");
	for	(i = 0; i < NOFILE; i++)
		{
		if	(i%8 == 0) printf("\t");
		printf("%.1x ", U.u_pofile[i]);
		if	(i%8 == 7) printf("\n");
		}
	if	(i%8) printf("\n");
	printf("lastfile\t%d\n", U.u_lastfile);
	printf("cdir\t%.1x\n", U.u_cdir);
	printf("rdir\t%.1x\n", U.u_rdir);
	printf("ttyp\t%.1x\n", U.u_ttyp);
	printf("ttyd\t%d,%d\n", major(U.u_ttyd), minor(U.u_ttyd));
	printf("cmask\t%.1x\n", U.u_cmask);
	printf("ru\t");
	ip = (long *)&U.u_ru;
	for	(i = 0; i < sizeof (U.u_ru) / sizeof (long); i++)
		printf("%ld ", ip[i]);
	printf("\n");
	printf("cru\t");
	ip = (long *)&U.u_cru;
	for	(i = 0; i < sizeof (U.u_cru) / sizeof (long); i++)
		printf("%ld ", ip[i]);
	printf("\n");
	printf("timer\t%ld %ld %ld %ld\n", U.u_timer[0].it_interval,
		U.u_timer[0].it_value, U.u_timer[1].it_interval,
		U.u_timer[1].it_value);
	printf("start\t%08x\n", U.u_start);
	printf("prof\t%.1x %u %u %u\n", U.u_prof.pr_base, U.u_prof.pr_size,
		U.u_prof.pr_off, U.u_prof.pr_scale);
	printf("rlimit cur\t");
	for	(i = 0; i < RLIM_NLIMITS; i++)
		{
		if	(U.u_rlimit[i].rlim_cur == RLIM_INFINITY)
			printf("infinite ");
		else
			printf("%ld ", U.u_rlimit[i].rlim_cur);
		}
	printf("\n");
	printf("rlimit max\t");
	for	(i = 0; i < RLIM_NLIMITS; i++)
		{
		if	(U.u_rlimit[i].rlim_max == RLIM_INFINITY)
			printf("infinite ");
		else
			printf("%ld ", U.u_rlimit[i].rlim_max);
		}
	printf("\n");
	printf("ncache\t%ld %u %d,%d\n", U.u_ncache.nc_prevoffset,
		U.u_ncache.nc_inumber, major(U.u_ncache.nc_dev),
		minor(U.u_ncache.nc_dev));
}

oatoi(s)
char *s;
{
	register v;

	v = 0;
	while (*s)
		v = (v<<3) + *s++ - '0';
	return(v);
}

dofile()
{
	struct file *xfile;
	register struct file *fp;
	register nf;
	u_int loc, afile;
	static char *dtypes[] = { "???", "inode", "socket", "pipe" };

	nf = 0;
	xfile = (struct file *)calloc(NFILE, sizeof (struct file));
	if (xfile == NULL) {
		fprintf(stderr, "can't allocate memory for file table\n");
		return;
	}
	afile = nl[SFIL].n_value;
	lseek(fc, (off_t)afile, 0);
	read(fc, xfile, NFILE * sizeof (struct file));
	for (fp=xfile; fp < &xfile[NFILE]; fp++)
		if (fp->f_count)
			nf++;
	if (totflg) {
		printf("%3d/%3d files\n", nf, NFILE);
		return;
	}
	printf("%d/%d open files\n", nf, NFILE);
	printf("   LOC   TYPE    FLG        CNT  MSG  DATA      OFFSET\n");
	loc = afile;
	for	(fp=xfile; fp < &xfile[NFILE]; fp++, loc += sizeof (*fp))
		{
		if (fp->f_count==0)
			continue;
		printf("%08x ", loc);
		if (fp->f_type <= DTYPE_PIPE)
			printf("%-8.8s", dtypes[fp->f_type]);
		else
			printf("8d", fp->f_type);
		putf((long)fp->f_flag&FREAD, 'R');
		putf((long)fp->f_flag&FWRITE, 'W');
		putf((long)fp->f_flag&FAPPEND, 'A');
		putf((long)fp->f_flag&FSHLOCK, 'S');
		putf((long)fp->f_flag&FEXLOCK, 'X');
		putf((long)fp->f_flag&FASYNC, 'I');
		putf((long)fp->f_flag&FNONBLOCK, 'n');
		putf((long)fp->f_flag&FMARK, 'm');
		putf((long)fp->f_flag&FDEFER, 'd');
		printf("  %3d", fp->f_count);
		printf("  %3d", fp->f_msgcount);
		printf("  %08x", fp->f_data);
		if (fp->f_offset < 0)
			printf("  0x%lx\n", fp->f_offset);
		else
			printf("  %ld\n", fp->f_offset);
	}
	free(xfile);
}

doswap()
{
	u_int	nswap, used;
	int	i, num;
	struct	map	smap;
	struct	mapent	*swp;

	nswap = getuint((off_t)nl[SNSWAP].n_value);

	lseek(fc, (off_t)nl[SWAPMAP].n_value, 0);
	read(fc, &smap, sizeof (smap));
	num = (smap.m_limit - smap.m_map);
	swp = (struct mapent *)calloc(num, sizeof (*swp));
	lseek(fc, (off_t)smap.m_map, 0);
	read(fc, swp, num * sizeof (*swp));
	for (used = 0, i = 0; swp[i].m_size; i++)
		used += swp[i].m_size;
	printf("%d/%d swapmap entries\n", i, num);
	printf("%u kbytes swap used, %u kbytes free\n", nswap - used, used);
}
