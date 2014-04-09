TOPSRC          = $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          += -O -Wall -Werror

OBJS    	= w5100.o socket.o ethernet.o client.o server.o udp.o

all:    	../libwiznet.a

../libwiznet.a: ${OBJS}
		@$(AR) ru $@ ${OBJS}
		$(RANLIB) $@

install: 	all

clean:
		rm -f *.o a.out core test errs ../libwiznet*.a
