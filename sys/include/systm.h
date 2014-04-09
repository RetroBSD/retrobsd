/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * The `securelevel' variable controls the security level of the system.
 * It can only be decreased by process 1 (/sbin/init).
 *
 * Security levels are as follows:
 *   -1	permannently insecure mode - always run system in level 0 mode.
 *    0	insecure mode - immutable and append-only flags make be turned off.
 *	All devices may be read or written subject to permission modes.
 *    1	secure mode - immutable and append-only flags may not be changed;
 *	raw disks of mounted filesystems, /dev/mem, and /dev/kmem are
 *	read-only.
 *    2	highly secure mode - same as (1) plus raw disks are always
 *	read-only whether mounted or not. This level precludes tampering
 *	with filesystems by unmounting them, but also inhibits running
 *	newfs while the system is secured.
 *
 * In normal operation, the system runs in level 0 mode while single user
 * and in level 1 mode while multiuser. If level 2 mode is desired while
 * running multiuser, it can be set in the multiuser startup script
 * (/etc/rc.local) using sysctl(8). If it is desired to run the system
 * in level 0 mode while multiuser, initialize the variable securelevel
 * in /sys/kern/kern_sysctl.c to -1. Note that it is NOT initialized to
 * zero as that would allow the vmunix binary to be patched to -1.
 * Without initialization, securelevel loads in the BSS area which only
 * comes into existence when the kernel is loaded and hence cannot be
 * patched by a stalking hacker.
 */
#include "conf.h"
extern int securelevel;		/* system security level */

extern const char version[];	/* system version */

/*
 * Nblkdev is the number of entries (rows) in the block switch.
 * Used in bounds checking on major device numbers.
 */
extern const int nblkdev;

/*
 * Number of character switch entries.
 */
extern const int nchrdev;

/*
 * Number of system call entries.
 */
extern const int nsysent;

extern int	mpid;			/* generic for unique process id's */
extern char	runin;			/* scheduling flag */
extern char	runout;			/* scheduling flag */
extern int	runrun;			/* scheduling flag */
extern char	curpri;			/* more scheduling */

extern u_int	swapstart, nswap;	/* start and size of swap space */
extern int	updlock;		/* lock for sync */
extern daddr_t	rablock;		/* block to be read ahead */
extern dev_t	rootdev;		/* device of the root */
extern dev_t	dumpdev;		/* device to take dumps on */
extern long	dumplo;			/* offset into dumpdev */
extern dev_t	swapdev;		/* swapping device */
extern dev_t	pipedev;		/* pipe device */

extern	const char icode[];             /* user init code */
extern	const char icodeend[];          /* its end */

struct inode;
daddr_t bmap (struct inode *ip, daddr_t bn, int rwflg, int flags);

extern void kmemdev();

/*
 * Structure of the system-entry table
 */
extern const struct sysent
{
	int	sy_narg;		/* total number of arguments */
	void	(*sy_call) (void);	/* handler */
} sysent[];

extern const char *syscallnames[];

extern int	noproc;			/* no one is running just now */
extern char	*panicstr;
extern int	boothowto;		/* reboot flags, from boot */
extern int	selwait;
extern size_t	physmem;		/* total amount of physical memory */

extern dev_t get_cdev_by_name(char *);
extern char *cdevname(dev_t dev);

void panic (char *msg);
void printf (char *fmt, ...);
void uprintf (char *fmt, ...);		/* print to the current user's terminal */
struct tty;
void tprintf (struct tty *tp, char *fmt, ...);	/* print to the specified terminal */
int loginit (void);
void log (int level, char *fmt, ...);
int logwrt (char *buf, int len, int log);
void logwakeup (int unit);
void cpuidentify (void);
void cninit (void);
void cnidentify (void);
void cnputc (char c);
int cngetc (void);
int baduaddr (caddr_t addr);		/* detect bad user address */
int badkaddr (caddr_t addr);		/* detect bad kernel address */

int strncmp (const char *s1, const char *s2, size_t n);
void bzero (void *s, size_t nbytes);
void bcopy (const void *src, void *dest, size_t nbytes);
int bcmp (const void *a, const void *b, size_t nbytes);
int copystr (caddr_t src, caddr_t dest, u_int maxlen, u_int *copied);
size_t strlen (const char *s);
int ffs (u_long i);			/* find the index of the lsb set bit */
void insque (void *element, void *pred);
void remque (void *element);

void startup (void);			/* machine-dependent startup code */
int chrtoblk (dev_t dev);		/* convert from character to block device number */
int blktochr (dev_t dev);		/* convert from block to character device number */
int isdisk (dev_t dev, int type);	/* determine if a device is a disk */
int iskmemdev (dev_t dev);		/* identify /dev/mem and /dev/kmem */
void boot (dev_t dev, int howto);

/*
 * Copy data from kernel space fromaddr to user space address toaddr.
 * Fromaddr and toaddr must be word aligned.  Returns zero on success,
 * EFAULT on failure.
 */
int copyout (const caddr_t from, caddr_t to, u_int nbytes);

/*
 * Copy data from user space fromaddr to kernel space address toaddr.
 * Fromaddr and toaddr must be word aligned.  Returns zero on success,
 * EFAULT on failure.
 */
int copyin (const caddr_t from, caddr_t to, u_int nbytes);

/*
 * Check if gid is a member of the group set.
 */
int groupmember (gid_t gid);

/*
 * Wake up all processes sleeping on chan.
 */
void wakeup (caddr_t chan);

/*
 * Allocate iostat disk monitoring slots for a driver.
 */
void dk_alloc (int *dkn, int slots, char *name);

/*
 * Initialize callouts.
 */
void coutinit (void);

/*
 * Syscalls.
 */
void	nosys (void);

/* 1.1 processes and protection */
void	getpid (void);
void	getppid (void), fork (void), rexit (void), execv (void), execve (void);
void	wait4 (void), getuid (void), getgid (void), getgroups (void), setgroups (void);
void	geteuid (void), getegid (void);
void	getpgrp (void), setpgrp (void);
void	setgid (void), setegid (void), setuid (void), seteuid (void);
void	ucall (void);					/* 2BSD calls */

/* 1.2 memory management */
void	brk (void);
void	ustore (void);                                  /* 2BSD calls */
void	ufetch (void);                                  /* 2BSD calls */

/* 1.3 signals */
void	sigstack (void), sigreturn (void);
void	sigaction (void), sigprocmask (void), sigpending (void), sigaltstack (void), sigsuspend (void);
void	sigwait (void), kill (void), killpg (void);

/* 1.4 timing and statistics */
void	gettimeofday (void), settimeofday (void);
void	getitimer (void), setitimer (void);
void	adjtime (void);

/* 1.5 descriptors */
void	getdtablesize (void), dup (void), dup2 (void), close (void);
void	pselect (void), select (void), fcntl (void), flock (void);

/* 1.6 resource controls */
void	getpriority (void), setpriority (void), getrusage (void), getrlimit (void), setrlimit (void);

/* 1.7 system operation support */
void	umount (void), smount (void);
void	sync (void), reboot (void), __sysctl (void);

/* 2.1 generic operations */
void	read (void), write (void), readv (void), writev (void), ioctl (void);

/* 2.2 file system */
void	chdir (void), fchdir (void), chroot (void);
void	mkdir (void), rmdir (void), chflags (void), fchflags (void);
void	open (void), mknod (void), unlink (void), stat (void), fstat (void), lstat (void);
void	chown (void), fchown (void), chmod (void), fchmod (void), utimes (void);
void	link (void), symlink (void), readlink (void), rename (void);
void	lseek (void), truncate (void), ftruncate (void), saccess (void), fsync (void);
void	statfs (void), fstatfs (void), getfsstat (void);

/* 2.3 communications */
void	socket (void), bind (void), listen (void), accept (void), connect (void);
void	socketpair (void), sendto (void), send (void), recvfrom (void), recv (void);
void	sendmsg (void), recvmsg (void), shutdown (void), setsockopt (void), getsockopt (void);
void	getsockname (void), getpeername (void), pipe (void);

void	umask (void);		/* XXX */

/* 2.4 processes */
void	ptrace (void);

void	profil (void);		/* 'cuz sys calls are interruptible */
void	vhangup (void);		/* should just do in exit (void) */
void	vfork (void);		/* awaiting fork w/ copy on write */

/*
 * Drivers.
 */
struct buf;
struct uio;

void cninit();
int cnopen (dev_t dev, int flag, int mode);
int cnclose (dev_t dev, int flag, int mode);
int cnread (dev_t dev, struct uio *uio, int flag);
int cnwrite (dev_t dev, struct uio *uio, int flag);
int cnioctl (dev_t dev, u_int cmd, caddr_t addr, int flag);
int cnselect (dev_t dev, int rw);

extern const struct devspec cndevs[];
extern const struct devspec mmdevs[];
extern const struct devspec sydevs[];
extern const struct devspec logdevs[];
extern const struct devspec fddevs[];

#ifdef TS_ISOPEN
extern struct tty cnttys[];
#endif

int mmrw (dev_t dev, struct uio *uio, int flag);
int seltrue (dev_t dev, int rw);
void nostrategy (struct buf *bp);
void nonet (void);

int syopen (dev_t dev, int flag, int mode);
int syread (dev_t dev, struct uio *uio, int flag);
int sywrite (dev_t dev, struct uio *uio, int flag);
int syioctl (dev_t dev, u_int cmd, caddr_t addr, int flag);
int syselect (dev_t dev, int rw);

int logopen (dev_t dev, int flag, int mode);
int logclose (dev_t dev, int flag, int mode);
int logread (dev_t dev, struct uio *uio, int flag);
int logioctl (dev_t dev, u_int cmd, caddr_t addr, int flag);
int logselect (dev_t dev, int rw);

int fdopen (dev_t dev, int flag, int mode);
int dupfdopen (int indx, int dfd, int mode, int error);
