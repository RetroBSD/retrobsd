#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	6.4 (Berkeley) 6/6/87
#

OBJS=	gethnamadr.o sethostent.o

SRCS=	gethnamadr.c sethostent.c

CFLAGS= -O ${DEFS}
TAGSFILE= tags
DESTDIR=

.c.o:
	${CC} -p -c ${CFLAGS} $*.c
	-ld -X -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	-ld -x -r $*.o
	mv a.out $*.o

hostlib hostlib_p: ${OBJS} 
	@echo "building profiled hostlib"
	@cd profiled; ar cru ../hostlib_p ${OBJS}
	@echo "building normal hostlib"
	@ar cru hostlib ${OBJS}
	
tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f *.o errs a.out core hostlib hostlib_p profiled/*.o tags

depend:
	mkdep ${CFLAGS} ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

gethnamadr.o: gethnamadr.c /usr/include/sys/param.h
gethnamadr.o: /usr/include/sys/localopts.h /usr/include/machine/machparam.h
gethnamadr.o: /usr/include/sys/types.h /usr/include/signal.h
gethnamadr.o: /usr/include/sys/types.h /usr/include/sys/socket.h
gethnamadr.o: /usr/include/netinet/in.h /usr/include/ctype.h
gethnamadr.o: /usr/include/netdb.h /usr/include/stdio.h /usr/include/errno.h
gethnamadr.o: /usr/include/arpa/inet.h /usr/include/arpa/nameser.h
gethnamadr.o: /usr/include/resolv.h
sethostent.o: sethostent.c /usr/include/sys/types.h /usr/include/arpa/nameser.h
sethostent.o: /usr/include/netinet/in.h /usr/include/resolv.h

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
