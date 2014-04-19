#
# This is the Makefile for the 'string' functions.  New routines ported from
# 4.4BSD's libc/string directory go here but existing libc/gen files are
# being left where they are.
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

DEFS		=
CFLAGS		+= ${DEFS}

SRCS		= strcspn.c strpbrk.c strerror.c strsep.c strspn.c strstr.c strtok.c strtok_r.c
OBJS		= strcspn.o strpbrk.o strerror.o strsep.o strspn.o strstr.o strtok.o strtok_r.o

all:		string.a

string.a:	${OBJS}
		@echo "building normal string.a"
		@ar cru string.a ${OBJS}

clean:
		rm -f *.o *~ profiled/*.o tags Makefile.bak string.a string_p.a

depend:		${SRCS}
		mkdep ${CFLAGS} ${SRCS}

tags:
		cwd=`pwd`; \
		for i in ${SRCS}; do \
			ctags -a -f ${TAGSFILE} $$cwd/$$i; \
		done

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
