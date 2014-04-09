TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS		+= -Werror -Wall

SRCS		= append.c ar.c archive.c contents.c delete.c extract.c misc.c \
                  move.c print.c replace.c strmode.c
OBJS            = append.o ar.o archive.o contents.o delete.o extract.o \
                  misc.o move.o print.o replace.o strmode.o
MAN		= ar.1.0 ar.5.0
MANSRC		= ar.1 ar.5

all:            ar $(MAN)

ar:             ${OBJS}
		${CC} ${LDFLAGS} -o ar.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S ar.elf > ar.dis
		${SIZE} ar.elf
		${ELF2AOUT} ar.elf $@ && rm ar.elf

ar.1.0:         ar.1
		${MANROFF} $< > $@

ar.5.0:         ar.5.5
		${MANROFF} $< > $@

clean:
		rm -f *.o *.0 *.elf ${MAN} ar *.elf *.dis tags *~

install:        all
		install ar $(DESTDIR)/bin/
		cp ar.1.0 $(DESTDIR)/share/man/cat1/ar.0
		cp ar.5.0 $(DESTDIR)/share/man/cat5/ar.0
