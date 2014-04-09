#	Makefile	5.11.3 (2.11BSD GTE)	1996/11/17
#
# LIBS can be set to reference other object modules and libraries needed
# by uucico, such as -lunet (needed when UNET is defined).
#
# Common object files are assembled into a library {uulib.a}
#
TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

ALL=	uucico uucp uux uuxqt uulog uuclean uuname uupoll uusnap \
	uuencode uudecode uusend uuq

LIBOBJ  = logent.o ulockf.o uucpdefs.o subdir.o gename.o assert.o expfile.o \
          chkpth.o cpmv.o uucpname.o getpwinfo.o versys.o xqt.o getargs.o \
          cfgets.o prefix.o lastpart.o getprm.o anyread.o anlwrk.o gnamef.o \
          mailst.o systat.o cntrl.o imsg.o gio.o sysacct.o pk0.o pk1.o \
          conn.o condevs.o chksum.o setline.o gnsys.o

CC      = $(GCCPREFIX)gcc -mips16 -EL -msoft-float -nostdinc -fshort-double -I$(TOPSRC)/include $(INCLUDES)



OWNER=	uucp
GROUP=	daemon

CFLAGS += -Os -Werror

SBINDIR=	${DESTDIR}/usr/sbin
LIBEXECDIR=	${DESTDIR}/usr/libexec
BIN=	${DESTDIR}/usr/bin
PUBDIR=	${DESTDIR}/usr/spool/uucppublic
SPOOL=	${DESTDIR}/usr/spool/uucp
XQTDIR=	${SPOOL}/XTMP
CORRUPT= ${SPOOL}/CORRUPT
AUDIT=	${SPOOL}/AUDIT
LCK= ${SPOOL}/LCK
LOG= ${SPOOL}/LOG
STST=	${SPOOL}/STST
# The six subdirs {not counting XTMP}.
HOSTNAME=`uuname -l | sed 's/\(.......\).*/\1/'`
SUBDIRS=C. D.${HOSTNAME}X D.${HOSTNAME} D. X. TM.

all:	libacu $(ALL)

libacu:
	cd aculib && make


uuencode: uuencode.o
	${CC} ${LDFLAGS} -o uuencode.elf uuencode.o ${LIBS}
	${OBJDUMP} -S uuencode.elf > uuencode.dis
	${SIZE} uuencode.elf
	${ELF2AOUT} uuencode.elf $@

uudecode: uudecode.o
	${CC} ${LDFLAGS} -o uudecode.elf uudecode.o ${LIBS}
	${OBJDUMP} -S uudecode.elf > uudecode.dis
	${SIZE} uudecode.elf
	${ELF2AOUT} uudecode.elf $@

uucp: uucp.o libuucp.a
	${CC} ${LDFLAGS} -o uucp.elf uucp.o -L. -luucp ${LIBS}
	${OBJDUMP} -S uucp.elf > uucp.dis
	${SIZE} uucp.elf
	${ELF2AOUT} uucp.elf $@

uux: uux.o libuucp.a
	${CC} ${LDFLAGS} -o uux.elf uux.o -L. -luucp ${LIBS}
	${OBJDUMP} -S uux.elf > uux.dis
	${SIZE} uux.elf
	${ELF2AOUT} uux.elf $@

uuxqt: uuxqt.o libuucp.a
	${CC} ${LDFLAGS} -o uuxqt.elf uuxqt.o -L. -luucp ${LIBS}
	${OBJDUMP} -S uuxqt.elf > uuxqt.dis
	${SIZE} uuxqt.elf
	${ELF2AOUT} uuxqt.elf $@

uulog: uulog.o libuucp.a
	${CC} ${LDFLAGS} -o uulog.elf uulog.o -L. -luucp ${LIBS}
	${OBJDUMP} -S uulog.elf > uulog.dis
	${SIZE} uulog.elf
	${ELF2AOUT} uulog.elf $@

uuclean: uuclean.o libuucp.a
	${CC} ${LDFLAGS} -o uuclean.elf uuclean.o -L. -luucp ${LIBS}
	${OBJDUMP} -S uuclean.elf > uuclean.dis
	${SIZE} uuclean.elf
	${ELF2AOUT} uuclean.elf $@

uuname: uuname.o libuucp.a
	${CC} ${LDFLAGS} -o uuname.elf uuname.o -L. -luucp ${LIBS}
	${OBJDUMP} -S uuname.elf > uuname.dis
	${SIZE} uuname.elf
	${ELF2AOUT} uuname.elf $@

uupoll: uupoll.o libuucp.a
	${CC} ${LDFLAGS} -o uupoll.elf uupoll.o -L. -luucp ${LIBS}
	${OBJDUMP} -S uupoll.elf > uupoll.dis
	${SIZE} uupoll.elf
	${ELF2AOUT} uupoll.elf $@

uusnap: uusnap.o libuucp.a
	${CC} ${LDFLAGS} -o uusnap.elf uusnap.o -L. -luucp ${LIBS}
	${OBJDUMP} -S uusnap.elf > uusnap.dis
	${SIZE} uusnap.elf
	${ELF2AOUT} uusnap.elf $@

uusend: uusend.o libuucp.a
	${CC} ${LDFLAGS} -o uusend.elf uusend.o -L. -luucp ${LIBS}
	${OBJDUMP} -S uusend.elf > uusend.dis
	${SIZE} uusend.elf
	${ELF2AOUT} uusend.elf $@

uuq: uuq.o libuucp.a
	${CC} ${LDFLAGS} -o uuq.elf uuq.o -L. -luucp ${LIBS}
	${OBJDUMP} -S uuq.elf > uuq.dis
	${SIZE} uuq.elf
	${ELF2AOUT} uuq.elf $@

uucico: cico.o libuucp.a
	${CC} ${LDFLAGS} -o uucico.elf cico.o -L. -Laculib -luucp -lacu ${LIBS}
	${OBJDUMP} -S uucico.elf > uucico.dis
	${SIZE} uucico.elf
	${ELF2AOUT} uucico.elf $@

libuucp.a: $(LIBOBJ)
	$(AR) r libuucp.a $(LIBOBJ)
	$(RANLIB) libuucp.a



install: uuencode uuencode
	cp $(ALL) $(TOPSRC)/bin

clean:
	rm -f *.o $(ALL) libuucp.a *.dis *.elf
	cd aculib; make ${MFLAGS} clean

depend:
	mkdep ${CFLAGS} *.c
