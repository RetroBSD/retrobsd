#
# Public domain - 1996/10/21 - sms
#
#  Makefile for misc data files (lib.b from 'bc', usw.)
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

SRCS		=
# eign lib.b Mail.help Mail.tildehelp units gprof.callg gprof.flat \
# vlpmacs mkproto.data

all:		${SRCS}

install:	${SRCS}
		-mkdir -p ${DESTDIR}/share/misc
		-chmod a+r,a+x ${DESTDIR}/share/misc
#		install -c -m 444 ${SRCS} ${DESTDIR}/share/misc

clean:
		rm -f *~
