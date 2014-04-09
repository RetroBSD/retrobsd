#	@(#)Makefile	1.0 (2.11BSD GTE) 4/5/94

DEFS= 	-DEXTENDED -I.
CFLAGS=	-O ${DEFS}
DESTDIR=
SEPFLAG= -i
SRCS=	eval.c expr.c look.c main.c misc.c
OBJS=	eval.o expr.o look.o main.o misc.o

all:	m4 tags

m4:	${OBJS}
	cc ${SEPFLAG} -o m4 ${OBJS}

install: all
	install -s -m 755 m4 ${DESTDIR}/usr/bin/m4

lint:
	lint -hax ${DEFS} ${SRCS}

tags:
	ctags -t *.h ${SRCS}

clean:
	-rm -f *.o m4 core

depend: ${SRCS}
	mkdep -p ${CFLAGS} ${SRCS}
