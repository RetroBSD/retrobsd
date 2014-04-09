# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# vmf package
#
TOPSRC          = $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFILES  	= vmf.c
OBJS    	= vmf.o
POBJS   	= ../vmf.o
MAN		= vmf.0
MANSRC		= vmf.3

CTAGS   	= ctags
#DEFS   	= -DNOSCCS
#CFLAGS 	= -O $(DEFS)
TAGSFILE	= tags

all:    	../libvmf.a $(MAN)

../libvmf.a: 	${OBJS}
		@echo building normal ../libvmf.a
		@$(AR) ru ../libvmf.a ${OBJS}
		$(RANLIB) ../libvmf.a

../libvmf_p.a: 	${OBJS}
		@echo building profiled ../libvmf.a
		@cd profiled; $(AR) ru ../../libvmf_p.a ${POBJS}
		$(RANLIB) ../libvmf_p.a

$(MAN):         $(MANSRC)
		${MANROFF} $< > $@

install: 	all
		cp $(MAN) $(DESTDIR)/share/man/cat3/
#		cp ../libvmf.a ${DESTDIR}/lib/libvmf.a
#		$(RANLIB) ${DESTDIR}/lib/libvmf.a
#		cp ../libvmf_p.a ${DESTDIR}/lib/libvmf_p.a
#		$(RANLIB) ${DESTDIR}/lib/libvmf_p.a

clean:
	rm -f *.o profiled/*.o a.out core test errs ../libvmf*.a vmf.0
