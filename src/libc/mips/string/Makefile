#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
TOPSRC		= $(shell cd ../../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= ${DEFS}

#
# Endian-independent assembly-code aliases for unaligned memory accesses.
# Little endian.
#
ASFLAGS		+= -DLWHI=lwr -DLWLO=lwl -DSWHI=swr -DSWLO=swl

SRCS		= bcopy.S bzero.S ffs.S memcpy.S memmove.S memset.S \
                  strlen.S bcmp.S index.S rindex.S strcmp.S
OBJS		= bcopy.o bzero.o ffs.o memcpy.o memmove.o memset.o \
                  strlen.o bcmp.o index.o rindex.o strcmp.o

string.a:	${OBJS}
		@echo "buiding string.a"
		@$(AR) cru string.a ${OBJS}

tags:
		cwd=`pwd`; \
		for i in ${SRCS}; do \
			ctags -a -f ${TAGSFILE} $$cwd/$$i; \
		done

clean:
		rm -f string.a *.o *~ profiled/*.o tags Makefile.bak

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
