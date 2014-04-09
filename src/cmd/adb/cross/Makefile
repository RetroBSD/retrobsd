TOPSRC		= $(shell cd ../../../..; pwd)
include $(TOPSRC)/cross.mk

vpath %.c ..

# Build a list of the host include directories.
CPP             = $(shell gcc -print-prog-name=cc1)
HOSTINC         = $(addprefix -I,$(shell echo | $(CPP) -v 2>&1 | grep '^ /.*include'))
HOSTINC         += -I/usr/include/i386-linux-gnu

CFLAGS		= -m32 -g -DCROSS -nostdinc -I. $(HOSTINC) -I.. -D__mips__
CFLAGS		+= -Wall -Werror -DNUM_SYMS_CACHE=50 -fno-builtin
LDFLAGS         += -m32 -g

OBJS            = message.o access.o command.o expr.o findfn.o format.o input.o \
                  main.o opset.o output.o pcs.o print.o \
                  runpcs.o setup.o sym.o
HEADERS         = a.out.h nlist.h time.h signal.h stdlib.h \
                  sys/exec.h sys/types.h sys/param.h sys/user.h \
                  sys/time.h sys/dir.h sys/resource.h sys/select.h \
                  sys/signal.h sys/wait.h \
                  machine/io.h machine/machparam.h

all:            $(HEADERS) adb

adb:            $(OBJS)
		$(CC) $(LDFLAGS) $(OBJS) -o $@

clean:
		rm -f *~ *.o adb *.h */*.h gccdump.s

$(HEADERS):
		ln -s -f ../../../../include/a.out.h .
		ln -s -f ../../../../include/nlist.h .
		ln -s -f ../../../../include/time.h .
		ln -s -f ../../../../include/stdlib.h .
		ln -s -f sys/signal.h .
		ln -s -f ../../../../../include/sys/exec.h sys/
		ln -s -f ../../../../../include/sys/param.h sys/
		ln -s -f ../../../../../include/sys/user.h sys/
		ln -s -f ../../../../../include/sys/types.h sys/
		ln -s -f ../../../../../include/sys/time.h sys/
		ln -s -f ../../../../../include/sys/dir.h sys/
		ln -s -f ../../../../../include/sys/resource.h sys/
		ln -s -f ../../../../../include/sys/select.h sys/
		ln -s -f ../../../../../include/sys/wait.h sys/
		ln -s -f ../../../../../include/sys/signal.h sys/
		ln -s -f ../../../../../include/machine/io.h machine/
		ln -s -f ../../../../../include/machine/machparam.h machine/

test:
		echo '$$q' | ./adb fsck fsck.core
		@printf '\n----------------------------\n'
		echo '$$m' | ./adb fsck fsck.core
		@printf '\n----------------------------\n'
