TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          += -O -Wall -Werror
LIBS            += -lwiznet -lc

EXAMPLES        = telnet \
                  ntpdate \
                  chat-server \
                  web-client \
                  web-server

all:            $(EXAMPLES)

web-client:     web-client.o
		${CC} ${LDFLAGS} -o $@.elf $< ${LIBS}
		${OBJDUMP} -S $@.elf > $@.dis
		${SIZE} $@.elf
		${ELF2AOUT} $@.elf $@

web-server:     web-server.o
		${CC} ${LDFLAGS} -o $@.elf $< ${LIBS}
		${OBJDUMP} -S $@.elf > $@.dis
		${SIZE} $@.elf
		${ELF2AOUT} $@.elf $@

telnet:         telnet.o
		${CC} ${LDFLAGS} -o $@.elf $< ${LIBS}
		${OBJDUMP} -S $@.elf > $@.dis
		${SIZE} $@.elf
		${ELF2AOUT} $@.elf $@

ntpdate:        ntpdate.o
		${CC} ${LDFLAGS} -o $@.elf $< ${LIBS}
		${OBJDUMP} -S $@.elf > $@.dis
		${SIZE} $@.elf
		${ELF2AOUT} $@.elf $@

chat-server:    chat-server.o
		${CC} ${LDFLAGS} -o $@.elf $< ${LIBS}
		${OBJDUMP} -S $@.elf > $@.dis
		${SIZE} $@.elf
		${ELF2AOUT} $@.elf $@

clean:
		rm -f *.o *.elf *.dis $(EXAMPLES)

install:        all
		install $(EXAMPLES) $(DESTDIR)/bin/
