TOPSRC          = $(shell cd ../../../..; pwd)
include $(TOPSRC)/target.mk

#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.3.2	(2.11BSD GTE)	1996/12/1
#
# make file for tip device drivers
#
# Current drivers:
#	BIZCOMP
#	USR COURIER (2400 baud)
#	DEC DF02-AC, DF03-AC
#	DEC DN-11/Able Quadracall
#	HAYES and Hayes emulators
#	PENRIL auto-dialer
#	VENTEL 212+ (w/o echo)
#	VADIC 831 RS232 adaptor
#	VADIC 3451
CFLAGS  += -I.. -Wall -Werror 
SRCS=	biz22.c biz31.c courier.c df.c dn11.c hayes.c ventel.c v831.c \
	v3451.c penril.c
OBJS=	biz22.o biz31.o courier.o df.o dn11.o hayes.o ventel.o v831.o \
	v3451.o penril.o

all: libacu.a

libacu.a: ${OBJS}
	$(AR) -rc libacu.a ${OBJS}
	$(RANLIB) libacu.a

clean: FRC
	rm -f ${OBJS} core libacu.a

depend: FRC
	mkdep ${CFLAGS} ${SRCS}

lint: FRC
	lint ${CFLAGS} ${SRCS}

tags: FRC
	ctags ${SRCS}

install:
FRC:

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
