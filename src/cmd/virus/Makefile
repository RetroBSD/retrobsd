TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          += -Wall -Werror -Os

virus:          virus.o
		$(LD) $(LDFLAGS) -o virus.elf virus.o $(LIBS) -ltermcap
		$(SIZE) virus.elf
		$(ELF2AOUT) virus.elf $@

install:        virus
		install virus ${DESTDIR}/bin/virus

clean:
		rm -f *.o virus.elf virus
