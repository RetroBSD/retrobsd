#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	crt0	Normal C run time startoff
#
TOPSRC          = $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk
vpath %.c $(TOPSRC)/src/startup-mips

#CFLAGS          = -Os -B$(TOPSRC)/lib/ -fno-integrated-as -Xassembler $(DEFS)
CFLAGS          = -Os

OBJS		= ../crt0.o

all:		$(OBJS)

../crt0.o:      crt0.s
		$(TOPSRC)/lib/as $< -o $@

crt0.s:         crt0.c
		$(CC) $(DEFS) $(CFLAGS) -S $< -o $@

clean:
		rm -f $(OBJS) *~
