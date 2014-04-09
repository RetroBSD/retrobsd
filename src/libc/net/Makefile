#
# Copyright (c) 1988 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and that due credit is given
# to the University of California at Berkeley. The name of the University
# may not be used to endorse or promote products derived from this
# software without specific prior written permission. This software
# is provided ``as is'' without express or implied warranty.
#
#	@(#)Makefile	5.16 (Berkeley) 3/14/88
#
SRCS=	getnbyaddr.c getnbyname.c getnent.c getpent.c getpname.c	\
	getproto.c getsbyname.c getsbyport.c getsent.c herror.c 	\
	rcmd.c res_comp.c res_debug.c res_init.c res_mkquery.c		\
	res_query.c res_send.c	\
	rexec.c ruserpass.c

OBJS=	getnbyaddr.o getnbyname.o getnent.o getpent.o getpname.o	\
	getproto.o getsbyname.o getsbyport.o getsent.o rcmd.o herror.o	\
	rcmd.o res_comp.o res_debug.o res_init.o res_mkquery.o		\
	res_query.o res_send.o rexec.o ruserpass.o

CFLAGS+= ${DEFS}
TAGSFILE=tags

.c.o:
	${CC} -p -c ${CFLAGS} $*.c
	-ld -X -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	-ld -x -r $*.o
	mv a.out $*.o

netlib netlib_p: ${OBJS}
	@echo "building profiled netlib"
	@cd profiled; ar cru ../netlib_p ${OBJS}
	@echo "building normal netlib"
	@ar cru netlib ${OBJS}

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f *.o profiled/*.o errs a.out core netlib netlib_p tags

depend:
	mkdep ${CFLAGS} ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

getnbyaddr.o: getnbyaddr.c /usr/include/netdb.h
getnbyname.o: getnbyname.c /usr/include/netdb.h
getnent.o: getnent.c /usr/include/stdio.h /usr/include/sys/types.h
getnent.o: /usr/include/sys/socket.h /usr/include/arpa/inet.h
getnent.o: /usr/include/netdb.h /usr/include/ctype.h
getpent.o: getpent.c /usr/include/stdio.h /usr/include/sys/types.h
getpent.o: /usr/include/sys/socket.h /usr/include/netdb.h /usr/include/ctype.h
getpname.o: getpname.c /usr/include/netdb.h
getproto.o: getproto.c /usr/include/netdb.h
getsbyname.o: getsbyname.c /usr/include/netdb.h
getsbyport.o: getsbyport.c /usr/include/netdb.h
getsent.o: getsent.c /usr/include/stdio.h /usr/include/sys/types.h
getsent.o: /usr/include/sys/socket.h /usr/include/netinet/in.h
getsent.o: /usr/include/netdb.h /usr/include/ctype.h
herror.o: herror.c /usr/include/sys/types.h /usr/include/sys/uio.h
rcmd.o: rcmd.c /usr/include/stdio.h /usr/include/ctype.h /usr/include/pwd.h
rcmd.o: /usr/include/sys/param.h /usr/include/sys/localopts.h
rcmd.o: /usr/include/machine/machparam.h /usr/include/sys/types.h
rcmd.o: /usr/include/signal.h /usr/include/sys/types.h /usr/include/sys/file.h
rcmd.o: /usr/include/sys/signal.h /usr/include/sys/socket.h
rcmd.o: /usr/include/sys/stat.h /usr/include/netinet/in.h /usr/include/netdb.h
rcmd.o: /usr/include/errno.h
res_comp.o: res_comp.c /usr/include/sys/types.h /usr/include/stdio.h
res_comp.o: /usr/include/arpa/nameser.h
res_debug.o: res_debug.c /usr/include/sys/types.h /usr/include/netinet/in.h
res_debug.o: /usr/include/stdio.h /usr/include/arpa/nameser.h
res_init.o: res_init.c /usr/include/sys/types.h /usr/include/sys/socket.h
res_init.o: /usr/include/netinet/in.h /usr/include/stdio.h
res_init.o: /usr/include/arpa/nameser.h /usr/include/resolv.h
res_mkquery.o: res_mkquery.c /usr/include/stdio.h /usr/include/sys/types.h
res_mkquery.o: /usr/include/netinet/in.h /usr/include/arpa/nameser.h
res_mkquery.o: /usr/include/resolv.h
res_query.o: res_query.c /usr/include/sys/param.h /usr/include/sys/localopts.h
res_query.o: /usr/include/machine/machparam.h /usr/include/sys/types.h
res_query.o: /usr/include/signal.h /usr/include/sys/types.h
res_query.o: /usr/include/sys/socket.h /usr/include/netinet/in.h
res_query.o: /usr/include/ctype.h /usr/include/netdb.h /usr/include/stdio.h
res_query.o: /usr/include/errno.h /usr/include/strings.h
res_query.o: /usr/include/arpa/inet.h /usr/include/arpa/nameser.h
res_query.o: /usr/include/resolv.h
res_send.o: res_send.c /usr/include/sys/param.h /usr/include/sys/localopts.h
res_send.o: /usr/include/machine/machparam.h /usr/include/sys/types.h
res_send.o: /usr/include/signal.h /usr/include/sys/types.h
res_send.o: /usr/include/sys/time.h /usr/include/time.h
res_send.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
res_send.o: /usr/include/netinet/in.h /usr/include/stdio.h /usr/include/errno.h
res_send.o: /usr/include/arpa/nameser.h /usr/include/resolv.h
rexec.o: rexec.c /usr/include/sys/types.h /usr/include/sys/socket.h
rexec.o: /usr/include/netinet/in.h /usr/include/stdio.h /usr/include/netdb.h
rexec.o: /usr/include/errno.h
ruserpass.o: ruserpass.c /usr/include/stdio.h /usr/include/utmp.h
ruserpass.o: /usr/include/ctype.h /usr/include/sys/types.h
ruserpass.o: /usr/include/sys/stat.h /usr/include/errno.h

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
