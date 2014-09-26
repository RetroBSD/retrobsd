#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation,
# advertising materials, and other materials related to such
# distribution and use acknowledge that the software was developed
# by the University of California, Berkeley.  The name of the
# University may not be used to endorse or promote products derived
# from this software without specific prior written permission.
# THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
# WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
#	@(#)Makefile	5.4.1 (2.11BSD) 1996/01/26
#
MDIR=	/usr/man/cat3f

.SUFFIXES: .3 .0
SRCS=	abort.3 access.3 alarm.3 bessel.3 bit.3 chdir.3 chmod.3 etime.3 \
	exit.3 fdate.3 flmin.3 flush.3 fork.3 fseek.3 getarg.3 getc.3 \
	getcwd.3 getenv.3 getlog.3 getpid.3 getuid.3 hostnm.3 idate.3 \
	index.3 intro.3 ioinit.3 kill.3 link.3 loc.3 long.3 malloc.3 \
	perror.3 plot.3 putc.3 qsort.3 rand.3 random.3 rename.3 signal.3 \
	sleep.3 stat.3 system.3 time.3 topen.3 traper.3 trapov.3 trpfpe.3 \
	ttynam.3 unlink.3 wait.3
OBJS=	abort.0 access.0 alarm.0 bessel.0 bit.0 chdir.0 chmod.0 etime.0 \
	exit.0 fdate.0 flmin.0 flush.0 fork.0 fseek.0 getarg.0 getc.0 \
	getcwd.0 getenv.0 getlog.0 getpid.0 getuid.0 hostnm.0 idate.0 \
	index.0 intro.0 ioinit.0 kill.0 link.0 loc.0 long.0 malloc.0 \
	perror.0 plot.0 putc.0 qsort.0 rand.0 random.0 rename.0 signal.0 \
	sleep.0 stat.0 system.0 time.0 topen.0 traper.0 trapov.0 trpfpe.0 \
	ttynam.0 unlink.0 wait.0

.3.0:
	${DESTDIR}/usr/man/manroff $*.3 > $*.0

all: _make_01

_make_01: ${OBJS}

clean: FRC
	rm -f ${OBJS}

install: _make_01
	install -c -o bin -g bin -m 444 ${OBJS} ${DESTDIR}${MDIR}
	rm -f ${DESTDIR}${MDIR}/and.0
	ln ${DESTDIR}${MDIR}/bit.0 ${DESTDIR}${MDIR}/and.0
	rm -f ${DESTDIR}${MDIR}/lshift.0
	ln ${DESTDIR}${MDIR}/bit.0 ${DESTDIR}${MDIR}/lshift.0
	rm -f ${DESTDIR}${MDIR}/not.0
	ln ${DESTDIR}${MDIR}/bit.0 ${DESTDIR}${MDIR}/not.0
	rm -f ${DESTDIR}${MDIR}/or.0
	ln ${DESTDIR}${MDIR}/bit.0 ${DESTDIR}${MDIR}/or.0
	rm -f ${DESTDIR}${MDIR}/rshift.0
	ln ${DESTDIR}${MDIR}/bit.0 ${DESTDIR}${MDIR}/rshift.0
	rm -f ${DESTDIR}${MDIR}/xor.0
	ln ${DESTDIR}${MDIR}/bit.0 ${DESTDIR}${MDIR}/xor.0
	rm -f ${DESTDIR}${MDIR}/arc.0
	ln ${DESTDIR}${MDIR}/plot.0 ${DESTDIR}${MDIR}/arc.0
	rm -f ${DESTDIR}${MDIR}/box.0
	ln ${DESTDIR}${MDIR}/plot.0 ${DESTDIR}${MDIR}/box.0
	rm -f ${DESTDIR}${MDIR}/circle.0
	ln ${DESTDIR}${MDIR}/plot.0 ${DESTDIR}${MDIR}/circle.0
	rm -f ${DESTDIR}${MDIR}/clospl.0
	ln ${DESTDIR}${MDIR}/plot.0 ${DESTDIR}${MDIR}/clospl.0
	rm -f ${DESTDIR}${MDIR}/cont.0
	ln ${DESTDIR}${MDIR}/plot.0 ${DESTDIR}${MDIR}/cont.0
	rm -f ${DESTDIR}${MDIR}/ctime.0
	ln ${DESTDIR}${MDIR}/time.0 ${DESTDIR}${MDIR}/ctime.0
	rm -f ${DESTDIR}${MDIR}/drand.0
	ln ${DESTDIR}${MDIR}/rand.0 ${DESTDIR}${MDIR}/drand.0
	rm -f ${DESTDIR}${MDIR}/erase.0
	ln ${DESTDIR}${MDIR}/plot.0 ${DESTDIR}${MDIR}/erase.0
	rm -f ${DESTDIR}${MDIR}/fgetc.0
	ln ${DESTDIR}${MDIR}/getc.0 ${DESTDIR}${MDIR}/fgetc.0
	rm -f ${DESTDIR}${MDIR}/fputc.0
	ln ${DESTDIR}${MDIR}/putc.0 ${DESTDIR}${MDIR}/fputc.0
	rm -f ${DESTDIR}${MDIR}/fstat.0
	ln ${DESTDIR}${MDIR}/stat.0 ${DESTDIR}${MDIR}/fstat.0
	rm -f ${DESTDIR}${MDIR}/gmtime.0
	ln ${DESTDIR}${MDIR}/time.0 ${DESTDIR}${MDIR}/gmtime.0
	rm -f ${DESTDIR}${MDIR}/irand.0
	ln ${DESTDIR}${MDIR}/rand.0 ${DESTDIR}${MDIR}/irand.0
	rm -f ${DESTDIR}${MDIR}/label.0
	ln ${DESTDIR}${MDIR}/plot.0 ${DESTDIR}${MDIR}/label.0
	rm -f ${DESTDIR}${MDIR}/line.0
	ln ${DESTDIR}${MDIR}/plot.0 ${DESTDIR}${MDIR}/line.0
	rm -f ${DESTDIR}${MDIR}/linemd.0
	ln ${DESTDIR}${MDIR}/plot.0 ${DESTDIR}${MDIR}/linemd.0
	rm -f ${DESTDIR}${MDIR}/lstat.0
	ln ${DESTDIR}${MDIR}/stat.0 ${DESTDIR}${MDIR}/lstat.0
	rm -f ${DESTDIR}${MDIR}/ltime.0
	ln ${DESTDIR}${MDIR}/time.0 ${DESTDIR}${MDIR}/ltime.0
	rm -f ${DESTDIR}${MDIR}/move.0
	ln ${DESTDIR}${MDIR}/plot.0 ${DESTDIR}${MDIR}/move.0
	rm -f ${DESTDIR}${MDIR}/openpl.0
	ln ${DESTDIR}${MDIR}/plot.0 ${DESTDIR}${MDIR}/openpl.0
	rm -f ${DESTDIR}${MDIR}/point.0
	ln ${DESTDIR}${MDIR}/plot.0 ${DESTDIR}${MDIR}/point.0
	rm -f ${DESTDIR}${MDIR}/short.0
	ln ${DESTDIR}${MDIR}/long.0 ${DESTDIR}${MDIR}/short.0
	rm -f ${DESTDIR}${MDIR}/space.0
	ln ${DESTDIR}${MDIR}/plot.0 ${DESTDIR}${MDIR}/space.0
	rm -f ${DESTDIR}${MDIR}/symlnk.0
	ln ${DESTDIR}${MDIR}/link.0 ${DESTDIR}${MDIR}/symlnk.0
	rm -f ${DESTDIR}${MDIR}/dffrac.0
	ln ${DESTDIR}${MDIR}/flmin.0 ${DESTDIR}${MDIR}/dffrac.0
	rm -f ${DESTDIR}${MDIR}/dflmax.0
	ln ${DESTDIR}${MDIR}/flmin.0 ${DESTDIR}${MDIR}/dflmax.0
	rm -f ${DESTDIR}${MDIR}/dflmin.0
	ln ${DESTDIR}${MDIR}/flmin.0 ${DESTDIR}${MDIR}/dflmin.0
	rm -f ${DESTDIR}${MDIR}/dtime.0
	ln ${DESTDIR}${MDIR}/etime.0 ${DESTDIR}${MDIR}/dtime.0
	rm -f ${DESTDIR}${MDIR}/ffrac.0
	ln ${DESTDIR}${MDIR}/flmin.0 ${DESTDIR}${MDIR}/ffrac.0
	rm -f ${DESTDIR}${MDIR}/flmax.0
	ln ${DESTDIR}${MDIR}/flmin.0 ${DESTDIR}${MDIR}/flmax.0
	rm -f ${DESTDIR}${MDIR}/fortran.0
	ln ${DESTDIR}${MDIR}/intro.0 ${DESTDIR}${MDIR}/fortran.0
	rm -f ${DESTDIR}${MDIR}/ftell.0
	ln ${DESTDIR}${MDIR}/fseek.0 ${DESTDIR}${MDIR}/ftell.0
	rm -f ${DESTDIR}${MDIR}/inmax.0
	ln ${DESTDIR}${MDIR}/flmin.0 ${DESTDIR}${MDIR}/inmax.0
	rm -f ${DESTDIR}${MDIR}/itime.0
	ln ${DESTDIR}${MDIR}/idate.0 ${DESTDIR}${MDIR}/itime.0
	rm -f ${DESTDIR}${MDIR}/len.0
	ln ${DESTDIR}${MDIR}/index.0 ${DESTDIR}${MDIR}/len.0
	rm -f ${DESTDIR}${MDIR}/lnblnk.0
	ln ${DESTDIR}${MDIR}/index.0 ${DESTDIR}${MDIR}/lnblnk.0
	rm -f ${DESTDIR}${MDIR}/rindex.0
	ln ${DESTDIR}${MDIR}/index.0 ${DESTDIR}${MDIR}/rindex.0
	rm -f ${DESTDIR}${MDIR}/tclose.0
	ln ${DESTDIR}${MDIR}/topen.0 ${DESTDIR}${MDIR}/tclose.0
	rm -f ${DESTDIR}${MDIR}/tread.0
	ln ${DESTDIR}${MDIR}/topen.0 ${DESTDIR}${MDIR}/tread.0
	rm -f ${DESTDIR}${MDIR}/trewin.0
	ln ${DESTDIR}${MDIR}/topen.0 ${DESTDIR}${MDIR}/trewin.0
	rm -f ${DESTDIR}${MDIR}/tskipf.0
	ln ${DESTDIR}${MDIR}/topen.0 ${DESTDIR}${MDIR}/tskipf.0
	rm -f ${DESTDIR}${MDIR}/tstate.0
	ln ${DESTDIR}${MDIR}/topen.0 ${DESTDIR}${MDIR}/tstate.0
	rm -f ${DESTDIR}${MDIR}/twrite.0
	ln ${DESTDIR}${MDIR}/topen.0 ${DESTDIR}${MDIR}/twrite.0
	rm -f ${DESTDIR}${MDIR}/besj0.0
	ln ${DESTDIR}${MDIR}/bessel.0 ${DESTDIR}${MDIR}/besj0.0
	rm -f ${DESTDIR}${MDIR}/besj1.0
	ln ${DESTDIR}${MDIR}/bessel.0 ${DESTDIR}${MDIR}/besj1.0
	rm -f ${DESTDIR}${MDIR}/besjn.0
	ln ${DESTDIR}${MDIR}/bessel.0 ${DESTDIR}${MDIR}/besjn.0
	rm -f ${DESTDIR}${MDIR}/besy0.0
	ln ${DESTDIR}${MDIR}/bessel.0 ${DESTDIR}${MDIR}/besy0.0
	rm -f ${DESTDIR}${MDIR}/besy1.0
	ln ${DESTDIR}${MDIR}/bessel.0 ${DESTDIR}${MDIR}/besy1.0
	rm -f ${DESTDIR}${MDIR}/besyn.0
	ln ${DESTDIR}${MDIR}/bessel.0 ${DESTDIR}${MDIR}/besyn.0
	rm -f ${DESTDIR}${MDIR}/dbesj0.0
	ln ${DESTDIR}${MDIR}/bessel.0 ${DESTDIR}${MDIR}/dbesj0.0
	rm -f ${DESTDIR}${MDIR}/dbesj1.0
	ln ${DESTDIR}${MDIR}/bessel.0 ${DESTDIR}${MDIR}/dbesj1.0
	rm -f ${DESTDIR}${MDIR}/dbesjn.0
	ln ${DESTDIR}${MDIR}/bessel.0 ${DESTDIR}${MDIR}/dbesjn.0
	rm -f ${DESTDIR}${MDIR}/dbesy0.0
	ln ${DESTDIR}${MDIR}/bessel.0 ${DESTDIR}${MDIR}/dbesy0.0
	rm -f ${DESTDIR}${MDIR}/dbesy1.0
	ln ${DESTDIR}${MDIR}/bessel.0 ${DESTDIR}${MDIR}/dbesy1.0
	rm -f ${DESTDIR}${MDIR}/dbesyn.0
	ln ${DESTDIR}${MDIR}/bessel.0 ${DESTDIR}${MDIR}/dbesyn.0
	rm -f ${DESTDIR}${MDIR}/drandm.0
	ln ${DESTDIR}${MDIR}/random.0 ${DESTDIR}${MDIR}/drandm.0
	rm -f ${DESTDIR}${MDIR}/falloc.0
	ln ${DESTDIR}${MDIR}/malloc.0 ${DESTDIR}${MDIR}/falloc.0
	rm -f ${DESTDIR}${MDIR}/fpecnt.0
	ln ${DESTDIR}${MDIR}/trpfpe.0 ${DESTDIR}${MDIR}/fpecnt.0
	rm -f ${DESTDIR}${MDIR}/free.0
	ln ${DESTDIR}${MDIR}/malloc.0 ${DESTDIR}${MDIR}/free.0
	rm -f ${DESTDIR}${MDIR}/gerror.0
	ln ${DESTDIR}${MDIR}/perror.0 ${DESTDIR}${MDIR}/gerror.0
	rm -f ${DESTDIR}${MDIR}/getgid.0
	ln ${DESTDIR}${MDIR}/getuid.0 ${DESTDIR}${MDIR}/getgid.0
	rm -f ${DESTDIR}${MDIR}/iargc.0
	ln ${DESTDIR}${MDIR}/getarg.0 ${DESTDIR}${MDIR}/iargc.0
	rm -f ${DESTDIR}${MDIR}/ierrno.0
	ln ${DESTDIR}${MDIR}/perror.0 ${DESTDIR}${MDIR}/ierrno.0
	rm -f ${DESTDIR}${MDIR}/irandm.0
	ln ${DESTDIR}${MDIR}/random.0 ${DESTDIR}${MDIR}/irandm.0
	rm -f ${DESTDIR}${MDIR}/isatty.0
	ln ${DESTDIR}${MDIR}/ttynam.0 ${DESTDIR}${MDIR}/isatty.0

FRC:
