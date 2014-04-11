#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= ${DEFS}

SRCS		= creat.c ftime.c gethostid.c gtty.c memccpy.c memchr.c memcmp.c \
		  memcpy.c memset.c nice.c pause.c rand.c sethostid.c \
		  setregid.c setreuid.c setrgid.c setruid.c sigcompat.c \
		  strchr.c strrchr.c stty.c times.c tmpnam.c utime.c
OBJS		= creat.o ftime.o gethostid.o gtty.o memccpy.o memchr.o memcmp.o \
		  memcpy.o memset.o nice.o pause.o rand.o sethostid.o \
		  setregid.o setreuid.o setrgid.o setruid.o sigcompat.o \
		  strchr.o strrchr.o stty.o times.o tmpnam.o utime.o

compat.a:	${OBJS}
		@echo "buiding compat.a"
		@$(AR) cru compat.a ${OBJS}

tags:
		cwd=`pwd`; \
		for i in ${SRCS}; do \
			ctags -a -f ${TAGSFILE} $$cwd/$$i; \
		done

clean:
		rm -f compat.a *.o *~ profiled/*.o tags Makefile.bak

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
