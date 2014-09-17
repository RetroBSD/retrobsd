TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS		+= -O -Werror -Wall
LIBS            = -lc

OBJS            = fortune.o
MAN		= fortune.0
MANSRC		= fortune.6

all:            fortune $(MAN) fortunes.dat

fortune:        $(OBJS)
		$(CC) $(LDFLAGS) -o fortune.elf $(OBJS) $(LIBS)
		$(OBJDUMP) -S fortune.elf > fortune.dis
		$(SIZE) fortune.elf
		$(ELF2AOUT) fortune.elf $@ && rm fortune.elf

strfile:        strfile.o
		$(CC) $(LDFLAGS) -o strfile strfile.o

unstr:          unstr.o
		$(CC) $(LDFLAGS) -o unstr unstr.o

fortunes.dat:   #fortunes strfile
		./strfile -r fortunes

fortunes:       scene obscene
		cp scene fortunes
		echo "%-" >> fortunes
		cat obscene >> fortunes

$(MAN):         $(MANSRC)
		$(MANROFF) $< > $@

clean:
		rm -f *.o *.0 *.elf $(MAN) fortune strfile unstr fortunes *.elf *.dis tags *~

install:        all
		install fortune $(DESTDIR)/games/
		cp $(MAN) $(DESTDIR)/share/man/cat6/
		cp fortunes.dat $(DESTDIR)/games/lib/

#$(OBJS):        fortune.h
