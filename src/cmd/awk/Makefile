TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror -Os

YACC            = bison -y
YFLAGS          = -d
LIBS            = -lm -lc
FILES           = awk.lx.o b.o main.o tran.o lib.o run.o parse.o proctab.o freeze.o
SOURCE          = awk.def.h awk.g.y awk.lx.l b.c lib.c main.c parse.c \
                  proc.c freeze.c run.c tran.c

awk:            $(FILES) awk.g.o
		${CC} ${LDFLAGS} -o awk.elf $(FILES) awk.g.o ${LIBS}
		${OBJDUMP} -S awk.elf > awk.dis
		${SIZE} awk.elf
		${ELF2AOUT} awk.elf $@ && rm awk.elf

y.tab.h:	awk.g.o

awk.h:          y.tab.h
		-cmp -s y.tab.h awk.h || cp y.tab.h awk.h

$(FILES):	awk.h awk.def.h

token.h:	awk.h tokenscript
		ex - < tokenscript

install:        awk
		install awk ${DESTDIR}/bin/

profile:	awk.g.o $(FILES) mon.o
		cc -p -i awk.g.o $(FILES) mon.o -lm

lint:
		lint -spu b.c main.c tran.c run.c lib.c parse.c -lm |\
			egrep -v '^(error|free|malloc)'

proctab.c:	proc.c token.h awk.h
		cc -Wall -Werror -o proc proc.c
		./proc > proctab.c

clean:
		-rm -f *.o *.tmp awk.lx.c *.out *.elf *.dis y.tab.* awk.h proc proctab.c token.h awk
