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
#	@(#)Makefile	5.4.3 (2.11BSD) 1997/5/31
#
DESTDIR=
MDIR=	${DESTDIR}/usr/man/cat4
SRCS=	acc.4 arp.4 bk.4 br.4 cons.4 css.4 de.4 dh.4 dhu.4 dhv.4 \
	dmc.4 dr.4 dz.4  \
	ec.4 en.4 fd.4 hk.4 ht.4 hy.4 icmp.4 idp.4 il.4 imp.4 impconf.4 inet.4 \
	intro.4 ip.4 lo.4 lp.4 mem.4 mtio.4 networking.4 ns.4 nsip.4 \
	null.4 pty.4 qe.4 ra.4 ram.4 rk.4 rl.4 rx.4 si.4 spp.4 sri.4 \
	swap.4 tb.4 tcp.4 tm.4 tmscp.4 ts.4 tty.4 udp.4 vv.4 xp.4
OBJS=	acc.0 arp.0 bk.0 br.0 cons.0 css.0 de.0 dh.0 dhu.0 dhv.0 \
	dmc.0 dr.0 \
	dz.0 ec.0 en.0 fd.0 hk.0 ht.0 hy.0 icmp.0 idp.0 il.0 imp.0 impconf.0 \
	inet.0 intro.0 ip.0 lo.0 lp.0 mem.0 mtio.0 networking.0 ns.0 \
	nsip.0 null.0 pty.0 qe.0 ra.0 ram.0 rk.0 rl.0 rx.0 si.0 spp.0 \
	sri.0 swap.0 tb.0 tcp.0 tm.0 tmscp.0 ts.0 tty.0 udp.0 vv.0 xp.0

.SUFFIXES: .4 .0

.4.0:
	${DESTDIR}/usr/man/manroff $*.4 > $*.0

all: ${OBJS}

clean: FRC
	rm -f ${OBJS}

install: all
	install -c -o bin -g bin -m 444 ${OBJS} ${MDIR}
	rm -f ${MDIR}/dvhp.0
	ln ${MDIR}/xp.0 ${MDIR}/dvhp.0
	rm -f ${MDIR}/hp.0
	ln ${MDIR}/xp.0 ${MDIR}/hp.0
	rm -f ${MDIR}/rm.0
	ln ${MDIR}/xp.0 ${MDIR}/rm.0
	rm -f ${MDIR}/rp.0
	ln ${MDIR}/xp.0 ${MDIR}/rp.0
	rm -f ${MDIR}/kmem.0
	ln ${MDIR}/mem.0 ${MDIR}/kmem.0
	rm -f ${MDIR}/stdin.0 rm -f ${MDIR}/stderr.0 rm -f ${MDIR}/stdout.0
	ln ${MDIR}/fd.0 ${MDIR}/stdin.0
	ln ${MDIR}/fd.0 ${MDIR}/stderr.0
	ln ${MDIR}/fd.0 ${MDIR}/stdout.0

FRC:
