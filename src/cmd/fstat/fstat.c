/*
 * fstat
 *
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */
#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/inode.h>
#if HAVE_NETWORK
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/unpcb.h>
#include <net/route.h>
#include <netinet/in_pcb.h>
#endif
#include <sys/vmmac.h>
#define	KERNEL
#include <sys/file.h>
#undef	KERNEL
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <nlist.h>
#include <pwd.h>

#define	N_KMEM	"/dev/kmem"
#define	N_MEM	"/dev/mem"
#define	N_SWAP	"/dev/swap"
#define	N_UNIX	"/vmunix"

#define	TEXT	-2
#define	WD	-1

typedef struct devs {
	struct devs	*next;
	dev_t	dev;
	int	inum;
	char	*name;
} DEVS;
DEVS	*devs;

static struct nlist nl[] = {
#define	X_PROC		0
	{ "_proc" },
#define	X_NPROC		1
	{ "_nproc" },
	{ "" },
};

struct proc	*mproc;

struct	user user;

extern int	errno;
static int	fflg, vflg;
static int	kmem, mem, nproc, swap;
static char	*uname;

static int
getu()
{
	if ((mproc->p_flag & SLOAD) == 0) {
		if (swap < 0)
			return(0);
		(void)lseek(swap, (off_t)(mproc->p_addr)<<9, L_SET);
		if (read(swap, (char *)&user, sizeof(struct user))
		    != sizeof(struct user)) {
			fprintf(stderr, "fstat: can't read u for pid %d from %s\n", mproc->p_pid, N_SWAP);
			return(0);
		}
		return(1);
	}
	(void)lseek(mem, mproc->p_addr, L_SET);
	if (read(mem, &user, sizeof(user))!=sizeof(user)) {
		printf("fstat: can't read page table for u of pid %d from %s\n", mproc->p_pid, N_MEM);
		return(0);
	}
	return(1);
}

static char *
itype(mode)
	u_short mode;
{
	switch(mode & IFMT) {
	case IFCHR:
		return("chr");
	case IFDIR:
		return("dir");
	case IFBLK:
		return("blk");
	case IFREG:
		return("reg");
	case IFLNK:
		return("lnk");
	case IFSOCK:
		return("soc");
	default:
		return("unk");
	}
	/*NOTREACHED*/
}

static int
devmatch(idev, inum, name)
	dev_t idev;
	ino_t inum;
	char  **name;
{
	register DEVS *d;

	for (d = devs; d; d = d->next)
		if (d->dev == idev && (d->inum == 0 || d->inum == inum)) {
			*name = d->name;
			return(1);
		}
	return(0);
}

static void
rerr1(what, fromwhat)
	char *what, *fromwhat;
{
	if (vflg)
		printf("fstat: error reading %s from %s", what, fromwhat);
}

static void
rerr2(err, address, what)
	int err, address;
	char *what;
{
	if (vflg)
		printf("error %d reading %s at %x from kmem\n", errno, what, address);
}

#if HAVE_NETWORK
/*
 * getinetproto --
 *	print name of protocol number
 */
static void
getinetproto(number)
	int number;
{
	char *cp;

	switch(number) {
	case IPPROTO_IP:
		cp = "ip"; break;
	case IPPROTO_ICMP:
		cp ="icmp"; break;
	case IPPROTO_GGP:
		cp ="ggp"; break;
	case IPPROTO_TCP:
		cp ="tcp"; break;
	case IPPROTO_EGP:
		cp ="egp"; break;
	case IPPROTO_PUP:
		cp ="pup"; break;
	case IPPROTO_UDP:
		cp ="udp"; break;
	case IPPROTO_IDP:
		cp ="idp"; break;
	case IPPROTO_RAW:
		cp ="raw"; break;
	default:
		printf(" %d", number);
		return;
	}
	printf(" %s", cp);
}

static
socktrans(sock)
	struct socket *sock;
{
	static char *stypename[] = {
		"unused",	/* 0 */
		"stream", 	/* 1 */
		"dgram",	/* 2 */
		"raw",		/* 3 */
		"rdm",		/* 4 */
		"seqpak"	/* 5 */
	};
#define	STYPEMAX 5
	struct socket	so;
	struct protosw	proto;
	struct domain	dom;
	struct inpcb	inpcb;
	struct unpcb	unpcb;
	int len;
	char dname[32];

	/* fill in socket */
	(void)lseek(kmem, (off_t)sock, L_SET);
	if (read(kmem, (char *)&so, sizeof(struct socket))
	    != sizeof(struct socket)) {
		rerr2(errno, (int)sock, "socket");
		return;
	}

	/* fill in protosw entry */
	(void)lseek(kmem, (off_t)so.so_proto, L_SET);
	if (read(kmem, (char *)&proto, sizeof(struct protosw))
	    != sizeof(struct protosw)) {
		rerr2(errno, (int)so.so_proto, "protosw");
		return;
	}

	/* fill in domain */
	(void)lseek(kmem, (off_t)proto.pr_domain, L_SET);
	if (read(kmem, (char *)&dom, sizeof(struct domain))
	    != sizeof(struct domain)) {
		rerr2(errno, (int)proto.pr_domain, "domain");
		return;
	}

	/*
	 * grab domain name
	 * kludge "internet" --> "inet" for brevity
	 */
	if (dom.dom_family == AF_INET)
		(void)strcpy(dname, "inet");
	else {
		(void)lseek(kmem, (off_t)dom.dom_name, L_SET);
		if ((len = read(kmem, dname, sizeof(dname) - 1)) < 0) {
			rerr2(errno, (int)dom.dom_name, "char");
			dname[0] = '\0';
		} else
			dname[len] = '\0';
	}

	if ((u_short)so.so_type > STYPEMAX)
		printf("* (%s unk%d %x", dname, so.so_type, so.so_state);
	else
		printf("* (%s %s %x", dname, stypename[so.so_type],
		    so.so_state);

	/*
	 * protocol specific formatting
	 *
	 * Try to find interesting things to print.  For tcp, the interesting
	 * thing is the address of the tcpcb, for udp and others, just the
	 * inpcb (socket pcb).  For unix domain, its the address of the socket
	 * pcb and the address of the connected pcb (if connected).  Otherwise
	 * just print the protocol number and address of the socket itself.
	 * The idea is not to duplicate netstat, but to make available enough
	 * information for further analysis.
	 */
	switch(dom.dom_family) {
	case AF_INET:
		getinetproto(proto.pr_protocol);
		if (proto.pr_protocol == IPPROTO_TCP ) {
			if (so.so_pcb) {
				(void)lseek(kmem, (off_t)so.so_pcb, L_SET);
				if (read(kmem, (char *)&inpcb, sizeof(struct inpcb))
				    != sizeof(struct inpcb)){
					rerr2(errno, (int)so.so_pcb, "inpcb");
					return;
				}
				printf(" %x", (int)inpcb.inp_ppcb);
			}
		}
		else if (so.so_pcb)
			printf(" %x", (int)so.so_pcb);
		break;
	case AF_UNIX:
		/* print address of pcb and connected pcb */
		if (so.so_pcb) {
			printf(" %x", (int)so.so_pcb);
			(void)lseek(kmem, (off_t)so.so_pcb, L_SET);
			if (read(kmem, (char *)&unpcb, sizeof(struct unpcb))
			    != sizeof(struct unpcb)){
				rerr2(errno, (int)so.so_pcb, "unpcb");
				return;
			}
			if (unpcb.unp_conn) {
				char shoconn[4], *cp;

				cp = shoconn;
				if (!(so.so_state & SS_CANTRCVMORE))
					*cp++ = '<';
				*cp++ = '-';
				if (!(so.so_state & SS_CANTSENDMORE))
					*cp++ = '>';
				*cp = '\0';
				printf(" %s %x", shoconn, (int)unpcb.unp_conn);
			}
		}
		break;
	default:
		/* print protocol number and socket address */
		printf(" %d %x", proto.pr_protocol, (int)sock);
	}
	printf(")\n");
}
#endif

static void
itrans(ftype, g, fno)
	int ftype, fno;
	struct inode	*g;		/* if ftype is inode */
{
	struct inode inode;
	dev_t idev;
	char *comm;
	char *name = (char *)NULL;	/* set by devmatch() on a match */

	if (g || fflg) {
		(void)lseek(kmem, (off_t)g, L_SET);
		if (read(kmem, (char *)&inode, sizeof(inode)) != sizeof(inode)) {
			rerr2(errno, (int)g, "inode");
			return;
		}
		idev = inode.i_dev;
		if (fflg && !devmatch(idev, inode.i_number, &name))
			return;
	}
	if (mproc->p_pid == 0)
		comm = "swapper";
	else
		comm = user.u_comm;
	printf("%-8.8s %-10.10s %5d  ", uname, comm, mproc->p_pid);

	switch(fno) {
	case WD:
		printf("  wd"); break;
	case TEXT:
		printf("text"); break;
	default:
		printf("%4d", fno);
	}

	if (g == 0) {
		printf("* (deallocated)\n");
		return;
	}

	switch(ftype) {
	case DTYPE_INODE:
	case DTYPE_PIPE:
		printf("\t%2d, %2d\t%5lu\t%6ld\t%3s %s\n", major(inode.i_dev),
		    minor(inode.i_dev), (long)inode.i_number,
		    inode.i_mode == IFSOCK ? 0L : inode.i_size,
		    ftype == DTYPE_PIPE ? "pip" :
		    itype(inode.i_mode), name ? name : "");
		break;
#if HAVE_NETWORK
	case DTYPE_SOCKET:
		socktrans((struct socket *)g);
		break;
#endif
	default:
		printf("* (unknown file type)\n");
	}
}

static void
readf()
{
	struct file lfile;
	int i;

	itrans(DTYPE_INODE, user.u_cdir, WD);
	for (i = 0; i < NOFILE; i++) {
		if (user.u_ofile[i] == 0)
			continue;
		(void)lseek(kmem, (off_t)user.u_ofile[i], L_SET);
		if (read(kmem, (char *)&lfile, sizeof(lfile))
		    != sizeof(lfile)) {
			rerr1("file", N_KMEM);
			continue;
		}
		itrans(lfile.f_type, (struct inode *)lfile.f_data, i);
	}
}

static int
getfname(filename)
	char *filename;
{
	struct stat statbuf;
	DEVS *cur;

	if (stat(filename, &statbuf)) {
		perror(filename);
		return(0);
	}
	if ((cur = (DEVS *)malloc(sizeof(DEVS))) == NULL) {
		fprintf(stderr, "fstat: out of space.\n");
		exit(1);
	}
	cur->next = devs;
	devs = cur;

	/* if file is block special, look for open files on it */
	if ((statbuf.st_mode & S_IFMT) != S_IFBLK) {
		cur->inum = statbuf.st_ino;
		cur->dev = statbuf.st_dev;
	}
	else {
		cur->inum = 0;
		cur->dev = statbuf.st_rdev;
	}
	cur->name = filename;
	return(1);
}

static long
lgetw(loc)
	off_t loc;
{
	u_int word;

	(void)lseek(kmem, (off_t)loc, L_SET);
	if (read(kmem, (char *)&word, sizeof(word)) != sizeof(word))
		rerr2(errno, (int)loc, "word");
	return((long)word);
}

static void
usage()
{
	fputs("usage: fstat [-v] [-u user] [-p pid] [filename ...]\n", stderr);
	exit(1);
}

int
main(argc, argv)
	int	argc;
	char	**argv;
{
	extern char *optarg;
	extern int optind;
	register struct passwd *passwd;
	register int pflg = 0, pid = 0, uflg = 0, uid = 0;
	int ch, size;

	while ((ch = getopt(argc, argv, "p:u:v")) != EOF)
		switch((char)ch) {
		case 'p':
			if (pflg++)
				usage();
			if (!isdigit(*optarg)) {
				fputs("fstat: -p option requires a process id.\n", stderr);
				usage();
			}
			pid = atoi(optarg);
			break;
		case 'u':
			if (uflg++)
				usage();
			if (!(passwd = getpwnam(optarg))) {
				fprintf(stderr, "%s: unknown uid\n", optarg);
				exit(1);
			}
			uid = passwd->pw_uid;
			uname = passwd->pw_name;
			break;
		case 'v':	/* undocumented: print read error messages */
			vflg++;
			break;
		case '?':
		default:
			usage();
		}

	if (*(argv += optind)) {
		for (; *argv; ++argv) {
			if (getfname(*argv))
				fflg = 1;
		}
		if (!fflg)	/* file(s) specified, but none accessable */
			exit(1);
	}

	if ((kmem = open(N_KMEM, O_RDONLY, 0)) < 0) {
		perror(N_KMEM);
		exit(1);
	}
	if ((mem = open(N_MEM, O_RDONLY, 0)) < 0) {
		perror(N_MEM);
		exit(1);
	}
	if ((swap = open(N_SWAP, O_RDONLY, 0)) < 0) {
		perror(N_SWAP);
		exit(1);
	}

	if (knlist(nl) == -1 || !nl[0].n_value) {
		fprintf(stderr, "%s: No namelist\n", N_UNIX);
		exit(1);
	}
	nproc = (int)lgetw((off_t)nl[X_NPROC].n_value);

	(void)lseek(kmem, (off_t)nl[X_PROC].n_value, L_SET);
	size = nproc * sizeof(struct proc);
	if ((mproc = (struct proc *)malloc((u_int)size)) == NULL) {
		fprintf(stderr, "fstat: out of space.\n");
		exit(1);
	}
	if (read(kmem, (char *)mproc, size) != size)
		rerr1("proc table", N_KMEM);

	printf("USER\t CMD\t      PID    FD\tDEVICE\tINODE\t  SIZE TYPE%s\n",
	    fflg ? " NAME" : "");
	for (; nproc--; ++mproc) {
		if (mproc->p_stat == 0)
			continue;
		if (pflg && mproc->p_pid != pid)
			continue;
		if (uflg)  {
			if (mproc->p_uid != uid)
				continue;
		}
		else
			uname = (passwd = getpwuid(mproc->p_uid)) ?
			    passwd->pw_name : "unknown";
		if (mproc->p_stat != SZOMB && getu() == 0)
			continue;
		readf();
	}
	exit(0);
}
