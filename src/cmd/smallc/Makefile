TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS          = -Os
LIBS		= -lc

ARCH            = mips

OBJS            = data.o error.o expr.o function.o gen.o initialise.o io.o lex.o main.o \
                  primary.o stmt.o sym.o while.o code$(ARCH).o #preproc.o

all:		smallc

smallc: 	$(OBJS)
		${CC} ${LDFLAGS} -o $@.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S $@.elf > $@.dis
		${SIZE} $@.elf
		${ELF2AOUT} $@.elf $@

install:        smallc
		cp smallc $(TOPSRC)/libexec/


clean:
		rm -f *.o smallc smallc.dis smallc.elf
###
code8080.o: code8080.c defs.h data.h
codeas09.o: codeas09.c defs.h data.h
codem68k.o: codem68k.c defs.h data.h
codemips.o: codemips.c defs.h data.h
codevax.o: codevax.c defs.h data.h
data.o: data.c defs.h
error.o: error.c defs.h data.h
expr.o: expr.c defs.h data.h
function.o: function.c defs.h data.h
gen.o: gen.c defs.h data.h
initialise.o: initialise.c defs.h data.h
io.o: io.c defs.h data.h
lex.o: lex.c defs.h data.h
main.o: main.c defs.h data.h
preproc.o: preproc.c defs.h data.h
primary.o: primary.c defs.h data.h
stmt.o: stmt.c defs.h data.h
sym.o: sym.c defs.h data.h
while.o: while.c defs.h data.h
