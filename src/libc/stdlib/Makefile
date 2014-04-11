#
# This is the Makefile for 'stdlib'.  New routines ported from 4.4BSD's
# libc/stdlib directory go here but existing libc/gen files are being left
# where they are.
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

DEFS		=
CFLAGS		+= ${DEFS}

SRCS		= getopt.c getsubopt.c strtol.c strtoul.c strtod.c
OBJS		= getopt.o getsubopt.o strtol.o strtoul.o strtod.o

all:		stdlib.a

stdlib.a:	${OBJS}
		@echo "building stdlib"
		@ar cru stdlib.a ${OBJS}

clean:
		rm -f *.o *~ profiled/*.o tags Makefile.bak stdlib.a stdlib_p.s

depend:		${SRCS}
		mkdep ${CFLAGS} ${SRCS}

tags:
		cwd=`pwd`; \
		for i in ${SRCS}; do \
			ctags -a -f ${TAGSFILE} $$cwd/$$i; \
		done

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
