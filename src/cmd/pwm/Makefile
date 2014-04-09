#
# Public Domain.  1995/03/13 - Steven Schultz
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror -I../../../sys/include

SRCS            = pwm.c
OBJS            = pwm.o
MAN             = pwm.0

all:            pwm ${MAN}

pwm:    ${OBJS}
		${CC} ${LDFLAGS} -o pwm.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S pwm.elf > pwm.dis
		${SIZE} pwm.elf
		${ELF2AOUT} pwm.elf $@

.SUFFIXES:      .0 .1

.1.0:
		${MANROFF} $*.1 > $@

clean:
		rm -f *.o *.elf ${MAN} *.elf *.dis tags *~ pwm

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		install -m 755 pwm ${DESTDIR}/bin

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}
