TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS		+= -Werror -Wall -g -I. -I../ar
LDFLAGS		+= -g

SRCS            = ../ar/archive.c ranlib.c
OBJS            = archive.o ranlib.o
MAN		= ranlib.0 ranlib.5.0
MANSRC		= ranlib.1

all:            ranlib $(MAN)

ranlib:         ${OBJS}
		${CC} ${LDFLAGS} -o ranlib.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S ranlib.elf > ranlib.dis
		${SIZE} ranlib.elf
		${ELF2AOUT} ranlib.elf $@

archive.o:      ../ar/archive.c
		${CC} ${CFLAGS} -c -o $@ $<

ranlib.0:       ranlib.1
		${MANROFF} $< > $@

ranlib.5.0: ranlib.5.5
		${MANROFF} $< > $@

clean:
		rm -f *.o *.0 *.elf ${MAN} ranlib *.elf *.dis a.out tags *~

install:
		cp ranlib.5.0 ${DESTDIR}/share/man/cat5/ranlib.0
		cp ranlib.0 ${DESTDIR}/share/man/cat1/ranlib.0
		install ranlib ${DESTDIR}/bin/ranlib
