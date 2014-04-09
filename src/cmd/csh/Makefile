#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley Software License Agreement
# specifies the terms and conditions for redistribution.
#
# C Shell with process control; VM/UNIX VAX Makefile
# Bill Joy UC Berkeley; Jim Kulp IIASA, Austria
#
# To profile, put -DPROF in DEFS and -pg in CFLAGS, and recompile.
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS          += -DTELL -DVFORK -DFILEC -DNOLIMITS -Os #-Werror -g

OBJS            = alloc.o printf.o sh.char.o sh.dir.o sh.dol.o \
                  sh.err.o sh.exp.o sh.file.o sh.func.o sh.glob.o sh.hist.o \
                  sh.init.o sh.lex.o sh.misc.o sh.parse.o sh.print.o sh.proc.o sh.sem.o \
                  sh.set.o sh.o sh.exec.o sh.time.o sh.exec2.o

all:		csh

csh:		${OBJS}
		${CC} ${LDFLAGS} -o csh.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S csh.elf > csh.dis
		${SIZE} csh.elf
		${ELF2AOUT} csh.elf $@ && rm csh.elf

install:	all
		install csh $(DESTDIR)/bin/
clean:
		rm -f csh *.o *~ *.elf *.dis

sh.o: sh.h sh.local.h sh.char.h
sh.char.o: sh.char.h
sh.dir.o: sh.h sh.local.h sh.dir.h
sh.dol.o: sh.h sh.local.h sh.char.h
sh.err.o: sh.h sh.local.h sh.char.h
sh.exec.o: sh.h sh.local.h sh.char.h
sh.exec2.o: sh.h sh.local.h sh.char.h
sh.exp.o: sh.h sh.local.h sh.char.h
sh.file.o: sh.h sh.local.h sh.char.h
sh.func.o: sh.h sh.local.h sh.char.h
sh.glob.o: sh.h sh.local.h sh.char.h
sh.hist.o: sh.h sh.local.h sh.char.h
sh.init.o: sh.local.h
sh.lex.o: sh.h sh.local.h sh.char.h
sh.misc.o: sh.h sh.local.h sh.char.h
sh.parse.o: sh.h sh.local.h sh.char.h
sh.print.o: sh.h sh.local.h sh.char.h
sh.proc.o: sh.h sh.local.h sh.dir.h sh.proc.h sh.char.h
sh.sem.o: sh.h sh.local.h sh.proc.h sh.char.h
sh.set.o: sh.h sh.local.h sh.char.h
sh.time.o: sh.h sh.local.h sh.char.h
