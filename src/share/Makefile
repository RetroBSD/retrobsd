#
# Public domain - 1996/10/21 - sms
#
#	@(#)Makefile	1.1 (2.11BSD) 1996/11/29
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

SUBDIR		= misc zoneinfo

all:		${SUBDIR}

${SUBDIR}: 	FRC
		cd $@; ${MAKE} ${MFLAGS}

FRC:

install: 	FRC
		-for i in ${SUBDIR}; do \
			(cd $$i; ${MAKE} ${MFLAGS} DESTDIR=${DESTDIR} install); done

clean:
		rm -f *~
		for i in ${SUBDIR}; do (cd $$i; ${MAKE} ${MFLAGS} clean); done
