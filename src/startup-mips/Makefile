#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	crt0	Normal C run time startoff
#
TOPSRC          = $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          = -O ${DEFS}

SRCS		= crt0.c
OBJS		= crt0.o

TAGSFILE	= tags

all:		${OBJS}
		cp crt0.o ../

install:	${OBJS}
#		$(INSTALLDIR) ${DESTDIR}/lib
#		$(INSTALL) crt0.o ${DESTDIR}/lib/crt0.o
#		$(INSTALL) mcrt0.o ${DESTDIR}/lib/mcrt0.o
#		$(INSTALL) gcrt0.o ${DESTDIR}/usr/lib/gcrt0.o

crt0.o:		crt0.c
		$(CC) ${DEFS} ${CFLAGS} -c $< -o $@

tags:
		cwd=`pwd`; \
		for i in *.c; do \
			ctags -a -f ${TAGSFILE} $$cwd/$$i; \
		done

clean:
		rm -f *.o *~ core errs tags Makefile.bak

depend:
		for i in ${SRCS}; do \
		    cc -M ${DEFS} ${DFLAGS} $$i | awk ' { if ($$1 != prev) \
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
