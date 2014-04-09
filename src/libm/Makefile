TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          += ${DEFS} -Werror -Wall

SRCS            = login.c logout.c logwtmp.c
OBJS            = asin.o atan.o exp.o erf.o floor.o fmod.o hypot.o j0.o j1.o \
                  jn.o log.o pow.o sin.o sinh.o sqrt.o tan.o tanh.o

all:            ../libm.a

../libm.a:	${OBJS}
		$(AR) cru $@ ${OBJS}
		$(RANLIB) $@

install:	all
#		${INSTALLDIR} ${DESTDIR}/lib
#		${INSTALL} ../libm.a ${DESTDIR}/lib/libm.a
#		$(RANLIB) ${DESTDIR}/lib/libm.a

clean:
		rm -f ../libm.a *.o *~ tags

cleandir:       clean
		rm -f .depend

depend:
		mkdep ${CFLAGS} ${SRCS}

tags:
		ctags ${SRCS}
