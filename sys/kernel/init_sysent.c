/*
 * System call switch table.
 *
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "systm.h"
#include "glob.h"

#ifdef INET
#   define ifnet(narg, name)	narg, name
#   define errnet(narg, name)	narg, name
#else
#   define ifnet(narg, name)	0, nosys
#   define errnet(narg, name)	0, nonet
#endif

extern void sc_msec();

/*
 * Reserved/unimplemented system calls in the range 0-150 inclusive
 * are reserved for use in future Berkeley releases.
 * Additional system calls implemented in vendor and other
 * redistributions should be placed in the reserved range at the end
 * of the current calls.
 */
/*
 * This table is the switch used to transfer to the appropriate routine for
 * processing a system call.  Each row contains the number of words of
 * arguments expected in registers, how many on the stack, and a pointer to
 * the routine.
 *
 * The maximum number of direct system calls is 255 since system call numbers
 * are encoded in the lower byte of the trap instruction -- see trap.c.
 */
const struct sysent sysent[] = {
	{ 1, nosys },			/*   0 = out-of-range */
	{ 1, rexit },			/*   1 = exit */
	{ 0, fork },			/*   2 = fork */
	{ 3, read },			/*   3 = read */
	{ 3, write },			/*   4 = write */
	{ 3, open },			/*   5 = open */
	{ 1, close },			/*   6 = close */
	{ 4, wait4 },			/*   7 = wait4 */
	{ 0, nosys },			/*   8 = (old creat) */
	{ 2, link },			/*   9 = link */
	{ 1, unlink },			/*  10 = unlink */
	{ 2, execv },			/*  11 = execv */
	{ 1, chdir },			/*  12 = chdir */
	{ 1, fchdir },			/*  13 = fchdir */
	{ 3, mknod },			/*  14 = mknod */
	{ 2, chmod },			/*  15 = chmod */
	{ 3, chown },			/*  16 = chown; now 3 args */
	{ 2, chflags },			/*  17 = chflags */
	{ 2, fchflags },		/*  18 = fchflags */
	{ 4, lseek },			/*  19 = lseek */
	{ 0, getpid },			/*  20 = getpid */
	{ 3, smount },			/*  21 = mount */
	{ 1, umount },			/*  22 = umount */
	{ 6, __sysctl },		/*  23 = __sysctl */
	{ 0, getuid },			/*  24 = getuid */
	{ 0, geteuid },			/*  25 = geteuid */
	{ 4, ptrace },			/*  26 = ptrace */
	{ 0, getppid },			/*  27 = getppid */
	{ 2, statfs },			/*  28 = statfs */
	{ 2, fstatfs },			/*  29 = fstatfs */
	{ 3, getfsstat },		/*  30 = getfsstat */
	{ 4, sigaction },		/*  31 = sigaction */
	{ 3, sigprocmask },		/*  32 = sigprocmask */
	{ 2, saccess },			/*  33 = access */
	{ 1, sigpending },		/*  34 = sigpending */
	{ 2, sigaltstack },		/*  35 = sigaltstack */
	{ 0, sync },			/*  36 = sync */
	{ 2, kill },			/*  37 = kill */
	{ 2, stat },			/*  38 = stat */
	{ 2, nosys },			/*  39 = getlogin */
	{ 2, lstat },			/*  40 = lstat */
	{ 1, dup },			/*  41 = dup */
	{ 0, pipe },			/*  42 = pipe */
	{ 1, nosys },			/*  43 = setlogin */
	{ 4, profil },			/*  44 = profil */
	{ 1, setuid },			/*  45 = setuid */
	{ 1, seteuid },			/*  46 = seteuid */
	{ 0, getgid },			/*  47 = getgid */
	{ 0, getegid },			/*  48 = getegid */
	{ 1, setgid },			/*  49 = setgid */
	{ 1, setegid },			/*  50 = setegid */
	{ 0, kmemdev },			/*  51 = kmemdev */
	{ 3, nosys },			/*  52 = (2.9) set phys addr */
	{ 1, nosys },			/*  53 = (2.9) lock in core */
	{ 4, ioctl },			/*  54 = ioctl */
	{ 1, reboot },			/*  55 = reboot */
	{ 2, sigwait },			/*  56 = sigwait */
	{ 2, symlink },			/*  57 = symlink */
	{ 3, readlink },		/*  58 = readlink */
	{ 3, execve },			/*  59 = execve */
	{ 1, umask },			/*  60 = umask */
	{ 1, chroot },			/*  61 = chroot */
	{ 2, fstat },			/*  62 = fstat */
	{ 0, nosys },			/*  63 = reserved */
	{ 0, nosys },			/*  64 = (old getpagesize) */
	{ 6, pselect },			/*  65 = pselect */
	{ 0, vfork },			/*  66 = vfork */
	{ 0, nosys },			/*  67 = unused */
	{ 0, nosys },			/*  68 = unused */
	{ 1, brk },			/*  69 = brk */
#ifdef GLOB_ENABLED
	{ 1, rdglob },			/*  70 = read from global */
	{ 2, wrglob },			/*  71 = write to global */
#else
    { 1, nosys },
    { 2, nosys },
#endif
	{ 0, sc_msec },			/*  72 = msec */
	{ 0, nosys },			/*  73 = unused */
	{ 0, nosys },			/*  74 = unused */
	{ 0, nosys },			/*  75 = unused */
	{ 0, vhangup },			/*  76 = vhangup */
	{ 0, nosys },			/*  77 = unused */
	{ 0, nosys },			/*  78 = unused */
	{ 2, getgroups },		/*  79 = getgroups */
	{ 2, setgroups },		/*  80 = setgroups */
	{ 1, getpgrp },			/*  81 = getpgrp */
	{ 2, setpgrp },			/*  82 = setpgrp */
	{ 3, setitimer },		/*  83 = setitimer */
	{ 0, nosys },			/*  84 = (old wait,wait3) */
	{ 0, nosys },			/*  85 = unused */
	{ 2, getitimer },		/*  86 = getitimer */
	{ 0, nosys },			/*  87 = (old gethostname) */
	{ 0, nosys },			/*  88 = (old sethostname) */
	{ 0, getdtablesize },		/*  89 = getdtablesize */
	{ 2, dup2 },			/*  90 = dup2 */
	{ 0, nosys },			/*  91 = unused */
	{ 3, fcntl },			/*  92 = fcntl */
	{ 5, select },			/*  93 = select */
	{ 0, nosys },			/*  94 = unused */
	{ 1, fsync },			/*  95 = fsync */
	{ 3, setpriority },		/*  96 = setpriority */
	{ errnet(3, socket) },		/*  97 = socket */
	{ ifnet(3, connect) },		/*  98 = connect */
	{ ifnet(3, accept) },		/*  99 = accept */
	{ 2, getpriority },		/* 100 = getpriority */
	{ ifnet(4, send) },		/* 101 = send */
	{ ifnet(4, recv) },		/* 102 = recv */
	{ 1, sigreturn },		/* 103 = sigreturn */
	{ ifnet(3, bind) },		/* 104 = bind */
	{ ifnet(5, setsockopt) },	/* 105 = setsockopt */
	{ ifnet(2, listen) },		/* 106 = listen */
	{ 1, sigsuspend },		/* 107 = sigsuspend */
	{ 0, nosys },			/* 108 = (old sigvec) */
	{ 0, nosys },			/* 109 = (old sigblock) */
	{ 0, nosys },			/* 110 = (old sigsetmask) */
	{ 0, nosys },			/* 111 = (old sigpause)  */
	{ 2, sigstack },		/* 112 = sigstack COMPAT-43 */
	{ ifnet(3, recvmsg) },		/* 113 = recvmsg */
	{ ifnet(3, sendmsg) },		/* 114 = sendmsg */
	{ 0, nosys },			/* 115 = unused */
	{ 2, gettimeofday },		/* 116 = gettimeofday */
	{ 2, getrusage },		/* 117 = getrusage */
	{ ifnet(5, getsockopt) },	/* 118 = getsockopt */
	{ 0, nosys },			/* 119 = unused */
	{ 3, readv },			/* 120 = readv */
	{ 3, writev },			/* 121 = writev */
	{ 2, settimeofday },		/* 122 = settimeofday */
	{ 3, fchown },			/* 123 = fchown */
	{ 2, fchmod },			/* 124 = fchmod */
	{ ifnet(6, recvfrom) },		/* 125 = recvfrom */
	{ 0, nosys },			/* 126 = (old setreuid) */
	{ 0, nosys },			/* 127 = (old setregid) */
	{ 2, rename },			/* 128 = rename */
	{ 3, truncate },		/* 129 = truncate */
	{ 3, ftruncate },		/* 130 = ftruncate */
	{ 2, flock },			/* 131 = flock */
	{ 0, nosys },			/* 132 = nosys */
	{ ifnet(6, sendto) },		/* 133 = sendto */
	{ ifnet(2, shutdown) },		/* 134 = shutdown */
	{ errnet(4, socketpair) },	/* 135 = socketpair */
	{ 2, mkdir },			/* 136 = mkdir */
	{ 1, rmdir },			/* 137 = rmdir */
	{ 2, utimes },			/* 138 = utimes */
	{ 0, nosys },			/* 139 = unused */
	{ 2, adjtime },			/* 140 = adjtime */
	{ ifnet(3, getpeername) },	/* 141 = getpeername */
	{ 0, nosys },			/* 142 = (old gethostid) */
	{ 0, nosys },			/* 143 = (old sethostid) */
	{ 2, getrlimit },		/* 144 = getrlimit */
	{ 2, setrlimit },		/* 145 = setrlimit */
	{ 2, killpg },			/* 146 = killpg */
	{ 0, nosys },			/* 147 = nosys */
	{ 2, nosys },			/* 148 = quota */
	{ 4, nosys },			/* 149 = qquota */
	{ ifnet(3, getsockname) },	/* 150 = getsockname */
	/*
	 * Syscalls 151-180 inclusive are reserved for vendor-specific
	 * system calls.  (This includes various calls added for compatibity
	 * with other Unix variants.)
	 */

	/*
	 * 2BSD special calls
	 */
	{ 0, nosys },			/* 151 = unused */
	{ 2, ustore },			/* 152 = ustore */
	{ 1, ufetch },			/* 153 = ufetch */
	{ 4, ucall },			/* 154 = ucall */
	{ 0, nosys },			/* 155 = fperr */
};

const int nsysent = sizeof (sysent) / sizeof (sysent[0]);
