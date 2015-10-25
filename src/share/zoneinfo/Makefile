# @(#)Makefile	1.6 Makefile 1997/12/28
TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

# If you want something other than Western United States time used on your
# system, change the line below (after finding the zone you want in the
# time zone files, or adding it to a time zone file).
# Alternately, if you discover you've got the wrong time zone, you can just
#	install -c -m 444 /share/zoneinfo/right_zone /etc/localtime

LOCALTIME=	US/Pacific

# If you want code inspired by certain emerging standards, add
#	-DSTD_INSPIRED
# to the end of the "CFLAGS=" line.
#
# If you want to handle solar-time-based time zones, remove the
# "#define NOSOLAR" from the include file usr/include/tzfile.h.
# (and add solar87 to the DATA= line below).

CFLAGS= -O
LINTFLAGS=	-phbaxc
CC=		cc -g -Wall -Werror

TZCSRCS=	zic.c scheck.c ialloc.c
TZCOBJS=	zic.o scheck.o ialloc.o
TZDSRCS=	zdump.c ialloc.c
TZDOBJS=	zdump.o ialloc.o
SOURCES=	zic.c zdump.c scheck.c ialloc.c
DATA=		asia australasia europe etcetera northamerica \
		pacificnew systemv
ZICMAN=		zic.0
ZICMANSRC=	zic.8
ZDUMAN=		zdump.0
ZDUMANSRC=	zdump.8
MAN=		$(ZICMAN) $(ZDUMAN)
MANSRC=		$(ZICMANSRC) $(ZDUMAN)
SHARDIR=	${DESTDIR}/share/zoneinfo
MANDIR=		${DESTDIR}/man/cat8

all:		tzfile.h zdump zic ${ZICMAN} ${ZDUMAN}

install:	zic $(DATA) $(MAN)
		(umask 22; ./zic -v -d ${SHARDIR} $(DATA))
		install -c ${SHARDIR}/${LOCALTIME} ${DESTDIR}/etc/localtime
#		install -c $(ZICMAN) ${MANDIR}/${ZICMAN}
#		install -c $(ZDUMAN) ${MANDIR}/${ZDUMAN}
#		install -c -s zic ${DESTDIR}/sbin/zic

zdump.0:	$(ZDUMANSRC)
		nroff -man $(ZDUMANSRC) > $(ZDUMAN)

zic.0:		$(ZICMANSRC)
		nroff -man $(ZICMANSRC) > $(ZICMAN)

zdump:		$(TZDOBJS)
		$(CC) $(CFLAGS) $(LFLAGS) $(TZDOBJS) -o $@

zic:		$(TZCOBJS)
		$(CC) $(CFLAGS) $(LFLAGS) $(TZCOBJS) -o $@

tzfile.h:       ${DESTDIR}/include/tzfile.h
		ln -s $< $@

lint:		$(TZCSRCS) $(TZDSRCS)
		lint $(LINTFLAGS) $(CFLAGS) $(TZCSRCS)
		lint $(LINTFLAGS) $(CFLAGS) $(TZDSRCS)

clean:
		rm -f *.o zdump zic $(ZICMAN) $(ZDUMAN) tzfile.h
