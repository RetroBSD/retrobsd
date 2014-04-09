#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= ${DEFS} -Os -Wall -Werror

SRCS		= ${STDSRC}
OBJS		= ${STDOBJ}

STDSRC		= abort.c alarm.c atof.c atoi.c atol.c calloc.c closedir.c crypt.c \
		  ctime.c ctype_.c daemon.c devname.c ecvt.c err.c \
		  execvp.c fakcu.c frexp.c fstab.c gcvt.c getenv.c getgrent.c \
                  getgrgid.c getgrnam.c getlogin.c \
		  getgrouplist.c gethostname.c getpagesize.c \
		  getpass.c getpwent.c getloadavg.c getmntinfo.c \
		  getttyent.c getttynam.c getusershell.c getwd.c \
		  initgroups.c isatty.c isinff.c isnanf.c ldexp.c malloc.c mktemp.c \
                  modff.c ndbm.c nlist.c knlist.c opendir.c perror.c popen.c \
                  psignal.c qsort.c random.c readdir.c regex.c scandir.c \
                  seekdir.c setmode.c sethostname.c setenv.c siglist.c \
                  signal.c siginterrupt.c sigsetops.c \
		  sleep.c strcasecmp.c strftime.c swab.c sysctl.c syslog.c system.c \
		  strcat.c strncat.c strcpy.c strncpy.c strncmp.c \
		  telldir.c time.c timezone.c ttyname.c ttyslot.c ualarm.c usleep.c \
		  strdup.c uname.c wait.c wait3.c waitpid.c

STDOBJ		= abort.o alarm.o atof.o atoi.o atol.o calloc.o closedir.o crypt.o \
		  ctime.o ctype_.o daemon.o devname.o ecvt.o err.o \
		  execvp.o fakcu.o frexp.o fstab.o gcvt.o getenv.o getgrent.o \
                  getgrgid.o getgrnam.o getlogin.o \
		  getgrouplist.o gethostname.o getpagesize.o \
		  getpass.o getpwent.o getloadavg.o getmntinfo.o \
		  getttyent.o getttynam.o getusershell.o getwd.o \
		  initgroups.o isatty.o isinff.o isnanf.o ldexp.o malloc.o mktemp.o \
                  modff.o ndbm.o nlist.o knlist.o opendir.o perror.o popen.o \
                  psignal.o qsort.o random.o readdir.o regex.o scandir.o \
                  seekdir.o setmode.o sethostname.o setenv.o siglist.o \
                  signal.o siginterrupt.o sigsetops.o \
		  sleep.o strcasecmp.o strftime.o swab.o sysctl.o syslog.o system.o \
		  strcat.o strncat.o strcpy.o strncpy.o strncmp.o \
		  telldir.o time.o timezone.o ttyname.o ttyslot.o ualarm.o usleep.o \
		  strdup.o uname.o wait.o wait3.o waitpid.o

gen.a:		${OBJS}
		@echo "buiding gen.a"
		@$(AR) cru gen.a ${OBJS}

tags:
		cwd=`pwd`; \
		for i in ${SRCS}; do \
			ctags -a -f ${TAGSFILE} $$cwd/$$i; \
		done

clean:
		rm -f gen.a *.o *~ profiled/*.o tags Makefile.bak

depend:
		for i in ${SRCS}; do \
		    cc -M ${CFLAGS} $$i | awk ' { if ($$1 != prev) \
			{ if (rec != "") print rec; rec = $$0; prev = $$1; } \
			else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
			else rec = rec " " $$2 } } \
			END { print rec } ' >> makedep; done
		echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
		echo '$$r makedep' >>eddep
		echo 'w' >>eddep
		cp Makefile Makefile.bak
		ed - Makefile < eddep
		rm eddep makedep
		echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
		echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
		echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it
