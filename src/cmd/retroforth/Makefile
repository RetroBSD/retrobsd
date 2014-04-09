TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror -Wall

SRCS		= retro.c
OBJS		= retro.o
BIN             = retroforth
LIBS            += -ltermcap

all:            $(BIN)

$(BIN):         ${SRCS}
		${CC} ${LDFLAGS} -o $(BIN).elf $(SRCS) ${LIBS}
		${SIZE} $(BIN).elf
		${OBJDUMP} -S $(BIN).elf > $(BIN).dis
		${ELF2AOUT} $(BIN).elf $@ && rm $(BIN).elf

clean:
		rm -f *.o *.elf $(BIN) *.elf *.dis *~

install:        all
		install $(BIN) $(DESTDIR)/bin/
		cp retroImage $(DESTDIR)/lib/

image:          $(BIN)
		cat image/meta.rx image/kernel.rx > core.rx
		./$(BIN) --shrink --image retroImage --with core.rx
		rm core.rx
