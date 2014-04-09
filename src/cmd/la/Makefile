#
# la Makefile.  Revised 1996/1/28
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror -Wall

SRCS		= la.c
OBJS		= la.o
MAN		= la.0
MANSRC		= la.1

all:            la $(MAN)

la:          	${OBJS}
		${CC} ${LDFLAGS} -o la.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S la.elf > la.dis
		${SIZE} la.elf
		${ELF2AOUT} la.elf $@ && rm la.elf

$(MAN):         ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf ${MAN} la *.elf *.dis tags *~

install:        all
		install la $(DESTDIR)/bin/
		cp $(MAN) $(DESTDIR)/share/man/cat1/
