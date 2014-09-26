#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation,
# advertising materials, and other materials related to such
# distribution and use acknowledge that the software was developed
# by the University of California, Berkeley.  The name of the
# University may not be used to endorse or promote products derived
# from this software without specific prior written permission.
# THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
# WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
#	@(#)Makefile	2.11 (2.11BSD) 2000/3/4
#
MDIR=	/usr/man/cat2
SRCS=	accept.2 access.2 acct.2 adjtime.2 bind.2 brk.2 chdir.2 chmod.2 \
	chown.2 chroot.2 close.2 connect.2 creat.2 dup.2 execve.2 exit.2 \
	chflags.2 \
	fcntl.2 fetchi.2 flock.2 fork.2 fperr.2 fsync.2 getdtablesize.2 \
	getgroups.2 gethostid.2 gethostname.2 getitimer.2 \
	getpagesize.2 getpeername.2 getpgrp.2 getpid.2 getpriority.2 \
	getrlimit.2 getrusage.2 getsockname.2 getsockopt.2 gettimeofday.2 \
	getfsstat.2 getlogin.2 \
	getuid.2 setreuid.2 setuid.2 getgid.2 setregid.2 \
	intro.2 ioctl.2 kill.2 killpg.2 link.2 listen.2 \
	lock.2 lseek.2 mkdir.2 mknod.2 mount.2 nostk.2 open.2 phys.2 \
	pipe.2 profil.2 ptrace.2 quota.2 read.2 readlink.2 reboot.2 \
	recv.2 rename.2 rmdir.2 select.2 send.2 setgroups.2 setpgrp.2 \
	setquota.2 shutdown.2 sigblock.2 sigpause.2 \
	sigaction.2 sigaltstack.2 sigpending.2 sigprocmask.2 sigsuspend.2 \
	sigreturn.2 sigsetmask.2 sigstack.2 sigwait.2 sigvec.2 socket.2 \
	socketpair.2 \
	stat.2 statfs.2 swapon.2 symlink.2 sync.2 syscall.2 truncate.2 ucall.2 \
	umask.2 unlink.2 utimes.2 vfork.2 vhangup.2 wait.2 write.2
OBJS=	accept.0 access.0 acct.0 adjtime.0 bind.0 brk.0 chdir.0 chmod.0 \
	chown.0 chroot.0 close.0 connect.0 creat.0 dup.0 execve.0 exit.0 \
	chflags.0 \
	fcntl.0 fetchi.0 flock.0 fork.0 fperr.0 fsync.0 getdtablesize.0 \
	getgroups.0 gethostid.0 gethostname.0 getitimer.0 \
	getpagesize.0 getpeername.0 getpgrp.0 getpid.0 getpriority.0 \
	getrlimit.0 getrusage.0 getsockname.0 getsockopt.0 gettimeofday.0 \
	getfsstat.0 getlogin.0 \
	getuid.0 setreuid.0 setuid.0 getgid.0 setregid.0 \
	intro.0 ioctl.0 kill.0 killpg.0 link.0 listen.0 \
	lock.0 lseek.0 mkdir.0 mknod.0 mount.0 nostk.0 open.0 phys.0 \
	pipe.0 profil.0 ptrace.0 quota.0 read.0 readlink.0 reboot.0 \
	recv.0 rename.0 rmdir.0 select.0 send.0 setgroups.0 setpgrp.0 \
	setquota.0 shutdown.0 sigblock.0 sigpause.0 \
	sigaction.0 sigaltstack.0 sigpending.0 sigprocmask.0 sigsuspend.0 \
	sigreturn.0 sigsetmask.0 sigstack.0 sigwait.0 sigvec.0 socket.0 \
	socketpair.0 \
	stat.0 statfs.0 swapon.0 symlink.0 sync.0 syscall.0 truncate.0 ucall.0 \
	umask.0 unlink.0 utimes.0 vfork.0 vhangup.0 wait.0 write.0

.SUFFIXES: .2 .0

.2.0:
	${DESTDIR}/usr/man/manroff $*.2 > $*.0

all: _make_01

_make_01: ${OBJS}

clean: FRC
	rm -f ${OBJS}

P=${DESTDIR}${MDIR}

REM= sbrk.0 dup2.0 fstat.0 lstat.0 readv.0 recvfrom.0 sendmsg.0 recvmsg.0 \
	sendto.0 fstatfs.0 wait3.0 wait4.0 waitpid.0 fchdir.0 fchflags.0 \
	fchmod.0 fchown.0 umount.0 writev.0 getppid.0 ftruncate.0 \
	pselect.0 \
	sethostid.0 setitimer.0 setrlimit.0 setsockopt.0 sethostname.0 \
	setpriority.0 settimeofday.0 setlogin.0 geteuid.0 setgid.0 \
	seteuid.0 setegid.0 getegid.0

install: _make_01
	install -c -o bin -g bin -m 444 ${OBJS} ${P}
	for i in ${REM}; do \
		rm -f  ${P}/$$i; \
	done
	ln ${P}/dup.0 ${P}/dup2.0
	ln ${P}/brk.0 ${P}/sbrk.0
	ln ${P}/stat.0 ${P}/fstat.0
	ln ${P}/stat.0 ${P}/lstat.0
	ln ${P}/read.0 ${P}/readv.0
	ln ${P}/recv.0 ${P}/recvfrom.0
	ln ${P}/recv.0 ${P}/recvmsg.0
	ln ${P}/send.0 ${P}/sendmsg.0
	ln ${P}/send.0 ${P}/sendto.0
	ln ${P}/statfs.0 ${P}/fstatfs.0
	ln ${P}/wait.0 ${P}/wait3.0
	ln ${P}/wait.0 ${P}/wait4.0
	ln ${P}/wait.0 ${P}/waitpid.0
	ln ${P}/chmod.0 ${P}/fchdir.0
	ln ${P}/chflags.0 ${P}/fchflags.0
	ln ${P}/chmod.0 ${P}/fchmod.0
	ln ${P}/chown.0 ${P}/fchown.0
	ln ${P}/mount.0 ${P}/umount.0
	ln ${P}/select.0 ${P}/pselect.0
	ln ${P}/write.0 ${P}/writev.0
	ln ${P}/getpid.0 ${P}/getppid.0
	ln ${P}/truncate.0 ${P}/ftruncate.0
	ln ${P}/gethostid.0 ${P}/sethostid.0
	ln ${P}/getitimer.0 ${P}/setitimer.0
	ln ${P}/getrlimit.0 ${P}/setrlimit.0
	ln ${P}/getsockopt.0 ${P}/setsockopt.0
	ln ${P}/gethostname.0 ${P}/sethostname.0
	ln ${P}/getpriority.0 ${P}/setpriority.0
	ln ${P}/gettimeofday.0 ${P}/settimeofday.0
	ln ${P}/getlogin.0 ${P}/setlogin.0
	ln ${P}/getuid.0 ${P}/geteuid.0
	ln ${P}/getgid.0 ${P}/getegid.0
	ln ${P}/setuid.0 ${P}/setgid.0
	ln ${P}/setuid.0 ${P}/seteuid.0
	ln ${P}/setuid.0 ${P}/setegid.0

FRC:
