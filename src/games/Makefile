#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS          += -Werror -Wall -Os

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR          = adventure atc backgammon battlestar boggle btlgammon \
                  caesar cribbage fortune hangman mille monop pom pig quiz \
                  robots rogue sail snake trek

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD             = banner aclock arithmetic bcd cfscores factor fish morse \
                  number ppt wump

# C programs that live in the current directory and need explicit make lines.
#
NSTD            = canfield primes rain worm worms

all:            $(SUBDIR) $(STD) $(NSTD)

$(SUBDIR):      FRC
		$(MAKE) -C $@ $(MFLAGS)

FRC:

# $(STD)
%: %.c
		$(CC) $(CFLAGS) $(LDFLAGS) -o $@.elf $< $(LIBS)
		$(OBJDUMP) -S $@.elf > $@.dis
		$(SIZE) $@.elf
		$(ELF2AOUT) $@.elf $@ && /bin/rm $@.elf

install:        $(STD) $(NSTD)
		-for i in $(SUBDIR); do \
			${MAKE} -C $$i $(MFLAGS) DESTDIR=$(DESTDIR) install; done
		-for i in $(STD) $(NSTD); do \
			install $$i $(DESTDIR)/games/$$i; done
		-for i in *.6; do \
			${MANROFF} $$i > $(DESTDIR)/share/man/cat6/`basename $$i .6`.0; done
		ln -f $(DESTDIR)/share/man/cat6/canfield.0 $(DESTDIR)/share/man/cat6/cfscores.0
		cat < /dev/null > $(DESTDIR)/games/lib/cfscores
		chmod 666 $(DESTDIR)/games/lib/cfscores

clean:
		rm -f a.out core *.s *.o *.dis *.elf $(STD) $(NSTD)
		-for i in $(SUBDIR); do ${MAKE} -C $$i $(MFLAGS) clean; done

# Files listed in $(NSTD) have explicit make lines given below.

canfield:       canfield.c
		$(CC) $(CFLAGS) $(LDFLAGS) -o $@.elf $< -lcurses -ltermcap $(LIBS)
		$(OBJDUMP) -S $@.elf > $@.dis
		$(SIZE) $@.elf
		$(ELF2AOUT) $@.elf $@ && /bin/rm $@.elf

primes:         primes.c
		$(CC) $(CFLAGS) $(LDFLAGS) -o $@.elf $< -lm $(LIBS)
		$(OBJDUMP) -S $@.elf > $@.dis
		$(SIZE) $@.elf
		$(ELF2AOUT) $@.elf $@ && /bin/rm $@.elf

rain:           rain.c
		$(CC) $(CFLAGS) $(LDFLAGS) -o $@.elf $< -ltermcap $(LIBS)
		$(OBJDUMP) -S $@.elf > $@.dis
		$(SIZE) $@.elf
		$(ELF2AOUT) $@.elf $@ && /bin/rm $@.elf

worm:           worm.c
		$(CC) $(CFLAGS) $(LDFLAGS) -o $@.elf $< -lcurses -ltermcap $(LIBS)
		$(OBJDUMP) -S $@.elf > $@.dis
		$(SIZE) $@.elf
		$(ELF2AOUT) $@.elf $@ && /bin/rm $@.elf

worms:          worms.c
		$(CC) $(CFLAGS) $(LDFLAGS) -o $@.elf $< -ltermcap $(LIBS)
		$(OBJDUMP) -S $@.elf > $@.dis
		$(SIZE) $@.elf
		$(ELF2AOUT) $@.elf $@ && /bin/rm $@.elf

# DO NOT DELETE THIS LINE -- make depend uses it
