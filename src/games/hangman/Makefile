TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS  	+= -Werror -Wall

OBJS            = endgame.o extern.o getguess.o getword.o main.o playgame.o \
		  prdata.o prman.o prword.o setup.o
MAN             = hangman.0
LIBS            = -lcurses -ltermcap -lc

all:            hangman $(MAN)

hangman:       $(OBJS)
		$(CC) $(LDFLAGS) -o $@.elf $(OBJS) $(LIBS)
		$(OBJDUMP) -S $@.elf > $@.dis
		$(SIZE) $@.elf
		$(ELF2AOUT) $@.elf $@ && rm $@.elf

$(MAN):         hangman.6
		nroff -man $< > $@

clean:
		rm -f *.o core hangman *.0 *.dis

install:        all
		install hangman $(DESTDIR)/games/
		install -m 644 $(MAN) $(DESTDIR)/share/man/cat6/
