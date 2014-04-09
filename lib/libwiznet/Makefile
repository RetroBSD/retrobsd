TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

vpath %.c $(TOPSRC)/src/libwiznet

CFLAGS		+= -B$(TOPSRC)/lib/ $(DEFS) -Wa,-x -Wall -Werror

OBJS		= w5100.o socket.o ethernet.o client.o server.o udp.o

all:            ../libwiznet.a

../libwiznet.a: ../ar ../ranlib $(OBJS)
		../ar rc $@ $(OBJS)
		../ranlib $@

clean:
		rm -f *~ *.o a.out *.a
