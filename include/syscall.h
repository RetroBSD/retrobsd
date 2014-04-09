/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * DO NOT place any comments on the same line as a SYS_* definition.  This
 * causes cpp to leave a trailing tab when expanding macros in pdp/sys/SYS.h
 */

#define	SYS_exit	1
#define	SYS_fork	2
#define	SYS_read	3
#define	SYS_write	4
#define	SYS_open	5
#define	SYS_close	6
#define	SYS_wait4	7
				/* 8 is old; creat */
#define	SYS_link	9
#define	SYS_unlink	10
#define	SYS_execv	11
#define	SYS_chdir	12
#define	SYS_fchdir	13
#define	SYS_mknod	14
#define	SYS_chmod	15
#define	SYS_chown	16
#define	SYS_chflags	17
#define	SYS_fchflags	18
#define	SYS_lseek	19
#define	SYS_getpid	20
#define	SYS_mount	21
#define	SYS_umount	22
#define	SYS___sysctl	23
#define	SYS_getuid	24
#define	SYS_geteuid	25
#define	SYS_ptrace	26
#define	SYS_getppid	27
#define	SYS_statfs	28
#define	SYS_fstatfs	29
#define	SYS_getfsstat	30
#define	SYS_sigaction	31
#define	SYS_sigprocmask	32
#define	SYS_access	33
#define	SYS_sigpending	34
#define	SYS_sigaltstack	35
#define	SYS_sync	36
#define	SYS_kill	37
#define	SYS_stat	38
				/* 39 was getlogin */
#define	SYS_lstat	40
#define	SYS_dup		41
#define	SYS_pipe	42
				/* 43 was setlogin */
#define	SYS_profil	44
#define	SYS_setuid	45
#define	SYS_seteuid	46
#define	SYS_getgid	47
#define	SYS_getegid	48
#define	SYS_setgid	49
#define	SYS_setegid	50
#define SYS_kmemdev 51
#define	SYS_phys	52
#define	SYS_lock	53
#define	SYS_ioctl	54
#define	SYS_reboot	55
#define	SYS_sigwait	56
#define	SYS_symlink	57
#define	SYS_readlink	58
#define	SYS_execve	59
#define	SYS_umask	60
#define	SYS_chroot	61
#define	SYS_fstat	62
				/* 63 is unused */
				/* 64 is old; getpagesize */
#define	SYS_pselect	65
#define	SYS_vfork	2       /* 66 - not fixed yet */
				/* 67 is old; vread */
				/* 68 is old; vwrite */
#define	SYS_sbrk	69
#define	SYS_rdglob	70
#define SYS_wrglob	71
				/* 71 is unused 4.3: mmap */
#define SYS_msec 	72	/* 72 is unused 4.3: vadvise */
				/* 73 is unused 4.3: munmap */
				/* 74 is unused 4.3: mprotect */
				/* 75 is unused 4.3: madvise */
#define	SYS_vhangup	76
				/* 77 is old; vlimit */
				/* 78 is unused 4.3: mincore */
#define	SYS_getgroups	79
#define	SYS_setgroups	80
#define	SYS_getpgrp	81
#define	SYS_setpgrp	82
#define	SYS_setitimer	83
				/* 84 is old; wait,wait3 */
#define	SYS_swapon	85
#define	SYS_getitimer	86
				/* 87 is old; gethostname */
				/* 88 is old; sethostname */
#define	SYS_getdtablesize 89
#define	SYS_dup2	90
				/* 91 is unused 4.3: getdopt */
#define	SYS_fcntl	92
#define	SYS_select	93
				/* 94 is unused 4.3: setdopt */
#define	SYS_fsync	95
#define	SYS_setpriority	96
#define	SYS_socket	97
#define	SYS_connect	98
#define	SYS_accept	99
#define	SYS_getpriority	100
#define	SYS_send	101
#define	SYS_recv	102
#define	SYS_sigreturn	103
#define	SYS_bind	104
#define	SYS_setsockopt	105
#define	SYS_listen	106
#define	SYS_sigsuspend	107
/*
 * 108 thru 112 are 4.3BSD compatibility syscalls.  sigstack has to remain
 * defined because no replacement routine exists.  Sigh.
*/
				/* 108 is old; sigvec */
				/* 109 is old; sigblock */
				/* 110 is old; sigsetmask */
				/* 111 is old; sigpause */
#define	SYS_sigstack	112

#define	SYS_recvmsg	113
#define	SYS_sendmsg	114
				/* 115 is old; vtrace */
#define	SYS_gettimeofday 116
#define	SYS_getrusage	117
#define	SYS_getsockopt	118
				/* 119 is old; resuba */
#define	SYS_readv	120
#define	SYS_writev	121
#define	SYS_settimeofday 122
#define	SYS_fchown	123
#define	SYS_fchmod	124
#define	SYS_recvfrom	125
				/* 126 is old; setreuid */
				/* 127 is old; setregid */
#define	SYS_rename	128
#define	SYS_truncate	129
#define	SYS_ftruncate	130
#define	SYS_flock	131
				/* 132 is unused */
#define	SYS_sendto	133
#define	SYS_shutdown	134
#define	SYS_socketpair	135
#define	SYS_mkdir	136
#define	SYS_rmdir	137
#define	SYS_utimes	138
				/* 139 is unused */
#define	SYS_adjtime	140
#define	SYS_getpeername	141
				/* 142 is old; gethostid */
				/* 143 is old; sethostid */
#define	SYS_getrlimit	144
#define	SYS_setrlimit	145
#define	SYS_killpg	146
				/* 147 is unused */
#define	SYS_setquota	148
#define	SYS_quota	149
#define	SYS_getsockname	150

/*
 * 2BSD special calls
 */
				/* 151 is unused */
#define	SYS_ustore	152
#define	SYS_ufetch	153
#define	SYS_ucall	154
				/* 155 is unused */
