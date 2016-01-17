TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          = -Os -Wall -DMIPS -DNO_ANNOTATIONS -DNO_PREPROCESSOR \
                  -DNO_PPACK -D_RETROBSD -D__SMALLER_C_SCHAR__ \
                  -D__SMALLER_C__ -D__SMALLER_C_32__ -DSTATIC \
                  -DNO_EXTRA_WARNS -DSYNTAX_STACK_MAX=3200

# For cross compile
#include $(TOPSRC)/cross.mk
#CFLAGS          = -Os -Wall -DMIPS -m32
#LDFLAGS         = -m32

LIBS		= -lc

ARCH            = mips

OBJS            = smlrc.o lb.o

all:		smlrc

smlrc:          $(OBJS)
		${CC} ${LDFLAGS} -o $@.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S $@.elf > $@.dis
		${SIZE} $@.elf
		${ELF2AOUT} $@.elf $@

install:        smlrc
		cp smlrc $(TOPSRC)/libexec/


clean:
		rm -f *.o smlrc smlrc.dis smlrc.elf
###
smlrc.o: smlrc.c fp.c cgmips.c
		${CC} ${CFLAGS} -mips16 smlrc.c -c -o $@
