#	Makefile for test
#
#	1.1 	Robin Birch	11 march 1995
#	1.2 	Steven Schultz	27 october 1996
#
#	Version 1.2
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS            = test.c operators.c
OBJS            = test.o operators.o
MAN             = test.0
MANSRC          = test.1

all:            test $(MAN)

test:        	${OBJS}
		${CC} ${LDFLAGS} -o test.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S test.elf > test.dis
		${SIZE} test.elf
		${ELF2AOUT} test.elf $@ && rm test.elf

$(MAN):         ${MANSRC}
		${MANROFF} $< > $@

clean:
		rm -f *.o *.elf ${MAN} test *.elf *.dis tags *~

install:        all
		install test ${DESTDIR}/bin/
		@rm -f ${DESTDIR}/bin/[
		ln ${DESTDIR}/bin/test ${DESTDIR}/bin/[
		cp $(MAN) ${DESTDIR}/share/man/cat1/
