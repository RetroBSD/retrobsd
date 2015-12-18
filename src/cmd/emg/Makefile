# emg Makefile
# for RetroBSD

TOPSRC =	$(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

# Some basic CFLAGS.
CFLAGS =	-Os -Wall -Werror

# With the extra LDFLAGS, save some bytes.
CFLAGS +=	-ffunction-sections -fdata-sections

# This reduces code size significantly.
CFLAGS +=	-mips16

# with CFLAGS+=	-ffunction-sections -fdata-sections
LDFLAGS +=	-Wl,--gc-sections

LIBS =		-ltermcap -lc

MAN =		emg.0
MANSRC =	emg.1

OBJS =	basic.o buffer.o display.o file.o fileio.o line.o main.o \
	random.o region.o search.o tcap.o ttyio.o word.o

all:	emg ${MAN}

emg:	${OBJS}
	${CC} ${LDFLAGS} -o emg.elf ${OBJS} ${LIBS}
	${OBJDUMP} -S emg.elf > emg.dis
	${SIZE} emg.elf
	${ELF2AOUT} emg.elf $@ && rm emg.elf

${MAN}:	${MANSRC}
	${MANROFF} ${MANSRC} > ${MAN}

install: all
	install emg ${DESTDIR}/bin/emg
	cp ${MAN} ${DESTDIR}/share/man/cat1/
	cp -p emg.keys ${DESTDIR}/share/emg.keys

clean:
	rm -f *.o *~ *.core *.bak *.dis emg ${MAN}
