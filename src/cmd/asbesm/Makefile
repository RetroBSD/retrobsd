#
# Assembler for BESM-6
#
DESTDIR = /usr/local
CFLAGS  = -I.. -O0 -Werror #-Wall
LDFLAGS =

as      = besm6-as

all:    as

as:     as.o
	$(CC) $(LDFLAGS) as.o -o as

install: $(DESTDIR)/bin/$(as)

clean:
	rm -f *.o *.b a.out core as

$(DESTDIR)/bin/$(as): as
	install -s as $(DESTDIR)/bin/$(as)

###
