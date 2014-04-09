#
# Makefile for ccom
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk
#CFLAGS		= -O -DCROSS

ARCHDIR         = arch-mips
MIPDIR          = mip
CFLAGS          += -DGCC_COMPAT -DPCC_DEBUG -D_ISOC99_SOURCE -DYY_NO_INPUT -Dmach_mips -I. -I${MIPDIR} -I${ARCHDIR}
CFLAGS		+= -g -Werror -Wall
LDFLAGS		+= -g

OBJS1           = optim.o pftn.o scan.o trees.o cgram.o inline.o symtabs.o \
                  gcc_compat.o init.o local.o code.o stabs.o builtins.o
OBJS2           = match.o reader.o optim2.o regs.o local2.o order.o table.o
OBJS            = $(OBJS1) $(OBJS2) compat.o common.o main.o external.o
MAN		= ccom.0
MANSRC		= ccom.1

all:            ccom $(MAN)

ccom:           ${OBJS}
		${CC} ${LDFLAGS} -o ccom.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S ccom.elf > ccom.dis
		${SIZE} ccom.elf
		${ELF2AOUT} ccom.elf $@ && rm ccom.elf

ccom.0:         ccom.1
		${MANROFF} $< > $@

clean:
		rm -f *.o *.0 *.elf ${MAN} ccom *.elf *.dis a.out *~ scan.c cgram.[ch] mkext

install:        all
		install ccom $(DESTDIR)/libexec/
		cp ccom.0 $(DESTDIR)/share/man/cat1/

local.o: $(ARCHDIR)/local.c external.h
		$(CC) $(CFLAGS) -c -o $@ $(ARCHDIR)/local.c

local2.o: $(ARCHDIR)/local2.c external.h
		$(CC) $(CFLAGS) -c -o $@ $(ARCHDIR)/local2.c

code.o: $(ARCHDIR)/code.c external.h
		$(CC) $(CFLAGS) -c -o $@ $(ARCHDIR)/code.c

order.o: $(ARCHDIR)/order.c external.h
		$(CC) $(CFLAGS) -c -o $@ $(ARCHDIR)/order.c

table.o: $(ARCHDIR)/table.c external.h
		$(CC) $(CFLAGS) -c -o $@ $(ARCHDIR)/table.c

match.o: $(MIPDIR)/match.c external.h
		$(CC) $(CFLAGS) -c -o $@ $(MIPDIR)/match.c

reader.o: $(MIPDIR)/reader.c external.h
		$(CC) $(CFLAGS) -c -o $@ $(MIPDIR)/reader.c

optim2.o: $(MIPDIR)/optim2.c external.h
		$(CC) $(CFLAGS) -c -o $@ $(MIPDIR)/optim2.c

regs.o: $(MIPDIR)/regs.c external.h
		$(CC) $(CFLAGS) -c -o $@ $(MIPDIR)/regs.c

cgram.o: cgram.c external.h
		$(CC) $(CFLAGS) -c -o $@ cgram.c

scan.o: scan.c external.h
		$(CC) $(CFLAGS) -c -o $@ scan.c

common.o: $(MIPDIR)/common.c external.h
		$(CC) $(CFLAGS) -c -o $@ $(MIPDIR)/common.c

compat.o: $(MIPDIR)/compat.c external.h
		$(CC) $(CFLAGS) -c -o $@ $(MIPDIR)/compat.c

external.h external.c: ${MIPDIR}/mkext.c $(ARCHDIR)/table.c
		gcc $(CFLAGS) -DMKEXT -o mkext ${MIPDIR}/mkext.c $(ARCHDIR)/table.c ${MIPDIR}/common.c
		./mkext

trees.c: 	mip/pass2.h external.h

inline.c: 	external.h
init.c: 	external.h
pftn.c: 	external.h
gcc_compat.c: 	external.h
symtabs.c: 	external.h
stabs.c: 	external.h
main.c: 	external.h

pass1.h: 	external.h
pass2.h: 	external.h

cgram.c: 	cgram.y
		$(YACC) $(YFLAGS) -d $<
		mv y.tab.c cgram.c
		mv y.tab.h cgram.h

scan.c: 	scan.l
		$(LEX) $(LFLAGS) $<
		mv lex.yy.c scan.c

optim2.o reader.o optim.o : external.h

pftn.o cgram.h: cgram.c
