TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS            = find.c bigram.c code.c

all:            find bigram code

find:           find.o
		${CC} ${LDFLAGS} -o find.elf $< ${LIBS}
		${OBJDUMP} -S find.elf > find.dis
		${SIZE} find.elf
		${ELF2AOUT} find.elf $@ && rm find.elf

bigram:         bigram.o
		${CC} ${LDFLAGS} -o bigram.elf $< ${LIBS}
		${OBJDUMP} -S bigram.elf > bigram.dis
		${SIZE} bigram.elf
		${ELF2AOUT} bigram.elf $@ && rm bigram.elf

code:           code.o
		${CC} ${LDFLAGS} -o code.elf $< ${LIBS}
		${OBJDUMP} -S code.elf > code.dis
		${SIZE} code.elf
		${ELF2AOUT} code.elf $@ && rm code.elf

clean:
		rm -f *.o *.elf find bigram code *.elf *.dis tags *~

install:        all
		install find ${DESTDIR}/bin/find
		install bigram ${DESTDIR}/libexec/bigram
		install code ${DESTDIR}/libexec/code
		install updatedb.sh ${DESTDIR}/sbin/updatedb

depend:
		chmod u+w Makefile
		mkdep ${CFLAGS} ${SRCS}
