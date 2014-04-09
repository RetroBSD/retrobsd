#	Makefile	4.4	86/02/12

TOPSRC          = $(shell cd ../../../..; pwd)
include $(TOPSRC)/target.mk

DEBUG           =
CFLAGS          = -Os -Werror ${DEBUG}

OBJS            = bsdtcp.o df2.o df12.o dk.o dn.o hys.o hysq.o mic.o nov.o pen.o pnet.o \
                  rvmacs.o sy.o unet.o va212.o va811.o va820.o vad.o vent.o vmacs.o \
                  hys24.o cds224.o att2224.o

libacu.a: ${OBJS}
		$(AR) cr libacu.a ${OBJS}
		-$(RANLIB) libacu.a

${OBJS}:        ../uucp.h ../condevs.h

install:        libacu.a

clean:
		rm -f *.o core errs  libacu.a
