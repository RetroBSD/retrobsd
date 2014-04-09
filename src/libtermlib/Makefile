#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
TOPSRC          = $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

DEFS    = -DNOSCCS
CFLAGS  = -O -DCM_N -DCM_GT -DCM_B -DCM_D $(DEFS) -Wall -Werror
LD      = $(CC)
SRCS    = termcap.c tgoto.c tputs.c tcattr.c
OBJS    = termcap.o tgoto.o tputs.o tcattr.o
POBJS   = ../termcap.o ../tgoto.o ../tputs.o ../tcattr.o
TAGSFILE= tags

#.c.o:
#	$(CC) $(CFLAGS) -c -p $*.c
#	$(LD) $(LDFLAGS) -x -r -o profiled/$*.o $*.o
#	$(CC) $(CFLAGS) -c $*.c
#	$(LD) $(LDFLAGS) -X -r $*.o
#	mv a.out $*.o

all:    ../libtermcap.a termcap

.PHONY: termcap

termcap:
	cd termcap && make

../libtermcap.a: ${OBJS}
	$(AR) cr ../libtermcap.a ${OBJS}

../libtermcap_p.a: ${OBJS}
	cd profiled; $(AR) cr ../../libtermcap_p.a ${POBJS}

install: all
#	cp ../libtermcap.a ${DESTDIR}/lib/libtermcap.a
#	@-rm -f ${DESTDIR}/lib/libtermlib.a
#	ln ${DESTDIR}/lib/libtermcap.a ${DESTDIR}/lib/libtermlib.a
#	$(RANLIB) ${DESTDIR}/lib/libtermcap.a
	$(MAKE) -C termcap install
#	cp ../libtermcap_p.a ${DESTDIR}/lib/libtermcap_p.a
#	@-rm -f ${DESTDIR}/lib/libtermlib_p.a
#	ln ${DESTDIR}/lib/libtermcap_p.a ${DESTDIR}/lib/libtermlib_p.a
#	$(RANLIB) ${DESTDIR}/lib/libtermcap_p.a

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	-rm -f *.o profiled/*.o
	-rm -f ../libtermcap.a ../libtermcap_p.a
	-rm -f termcap/termcap

VGRIND=	csh /usr/ucb/vgrind
vgrind:
	cp /dev/null index
	${VGRIND} -h "Termcap library" termcap.c tputs.c tgoto.c
	${VGRIND} -h "Termcap library" -x index
