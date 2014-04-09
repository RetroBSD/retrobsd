#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

SRCS		= inet_addr.c inet_network.c inet_netof.c \
		  inet_ntoa.c inet_lnaof.c inet_maddr.c
OBJS		= inet_addr.o inet_network.o inet_netof.o \
		  inet_ntoa.o inet_lnaof.o inet_maddr.o

CFLAGS		+= ${DEFS} -Os

inet.a: ${OBJS}
		@echo "building inet.a"
		@ar cru inet.a ${OBJS}

tags:
		cwd=`pwd`; \
		for i in ${SRCS}; do \
			ctags -a -f ${TAGSFILE} $$cwd/$$i; \
		done

clean:
		rm -f *.a *~ *.o profiled/*.o errs a.out core tags Makefile.bak

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
