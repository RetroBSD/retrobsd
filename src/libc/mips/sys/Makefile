TOPSRC		= $(shell cd ../../../..; pwd)
include $(TOPSRC)/target.mk

ASFLAGS		+= ${DEFS}

# modules which can not use SYSCALL and must be assembled from sources.  The
# rest of the system calls are generated with printf(1) and do not have
# source files associated with them.

COBJS		= sbrk.o execl.o execle.o execv.o

ASMOBJS		= _exit.o _brk.o pipe.o ptrace.o sigaction.o

SYSOBJS		= __sysctl.o accept.o access.o adjtime.o bind.o chdir.o \
		  chflags.o chmod.o chown.o chroot.o close.o connect.o dup.o \
		  dup2.o execve.o fchdir.o fchflags.o fchmod.o fchown.o \
		  fcntl.o flock.o fork.o fstat.o fsync.o ftruncate.o \
		  getdtablesize.o getgroups.o getitimer.o getsockname.o \
		  getpeername.o getpriority.o getrlimit.o getrusage.o getsockopt.o \
		  gettimeofday.o ioctl.o kill.o killpg.o link.o listen.o lstat.o mkdir.o \
		  mknod.o mount.o open.o pselect.o quota.o read.o readlink.o readv.o \
		  reboot.o \
		  recv.o recvfrom.o recvmsg.o rename.o rmdir.o select.o send.o sendmsg.o \
		  sendto.o setgroups.o setitimer.o setpgrp.o setpriority.o setquota.o \
		  setuid.o seteuid.o setgid.o setegid.o \
		  setrlimit.o setsockopt.o settimeofday.o shutdown.o \
		  sigaltstack.o socket.o socketpair.o stat.o symlink.o \
		  sigprocmask.o sigstack.o sigwait.o \
		  statfs.o fstatfs.o getfsstat.o \
		  truncate.o umount.o unlink.o utimes.o wait4.o write.o writev.o \
		  lseek.o sigsuspend.o \
		  getgid.o getegid.o getpgrp.o getpid.o \
		  getppid.o getuid.o geteuid.o profil.o sigpending.o sync.o \
                  ufetch.o ustore.o ucall.o umask.o vfork.o vhangup.o \
		  rdglob.o wrglob.o msec.o kmemdev.o

OBJS		= ${COBJS} ${ASMOBJS} ${SYSOBJS}

TAGSFILE	= tags

sys.a:		${OBJS}
		@echo "building sys.a"
		@$(AR) cru sys.a ${OBJS}

${SYSOBJS}:	SYS.h
		@echo creating $*.o
		@printf '#include "SYS.h"\nSYS($*)\n' | $(AS) ${ASFLAGS} - -c -o $*.o

clean:
		rm -f *~ *.o a.out sys.a
