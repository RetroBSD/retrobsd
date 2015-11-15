TOPSRC          = $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

vpath %.c $(TOPSRC)/src/libm

CFLAGS          = -Os -B$(TOPSRC)/lib/ $(DEFS) -Wa,-x -Wall -Werror

OBJS            = asin.o atan.o exp.o erf.o floor.o fmod.o hypot.o j0.o j1.o \
                  jn.o log.o pow.o sin.o sinh.o sqrt.o tan.o tanh.o

all:            ../libm.a

../libm.a: ../ar ../ranlib $(OBJS)
		../ar rc $@ $(OBJS)
		../ranlib $@

clean:
		rm -f *~ *.o a.out *.a
