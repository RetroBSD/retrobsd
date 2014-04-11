TOPSRC          = $(shell cd ..; pwd)
SUBDIR		= startup libc libcurses libtermlib libwiznet
PROG		= ar as aout ld nm ranlib size strip

# Build a list of the host include directories.
CPP             = $(shell gcc -print-prog-name=cc1)
HOSTINC         = $(addprefix -I,$(shell echo | $(CPP) -v 2>&1 | grep '^ /.*include'))
HOSTINC         += -I/usr/include/i386-linux-gnu

CFLAGS		+= -nostdinc -g -Werror -Wall -DCROSS -I. $(HOSTINC) \
                   -I$(TOPSRC)/include -I$(TOPSRC)/src/cmd/ar \
                   -I$(TOPSRC)/src/cmd/as
LDFLAGS         += -g

AR_OBJS         = ar.o append.o archive.o contents.o delete.o extract.o \
                  misc.o move.o print.o replace.o strmode.o
AOUT_OBJS       = aout.o mips-dis.o
RANLIB_OBJS     = ranlib.o archive.o
HEADERS         = a.out.h ar.h nlist.h ranlib.h

vpath %.c $(TOPSRC)/src/cmd/ar $(TOPSRC)/src/cmd/as $(TOPSRC)/src/cmd/ld \
    $(TOPSRC)/src/cmd/nm $(TOPSRC)/src/cmd/ranlib $(TOPSRC)/src/cmd

all install depend: $(HEADERS) $(SUBDIR) $(PROG)
		-for i in $(SUBDIR); do $(MAKE) -C $$i $(MFLAGS) DESTDIR=$(DESTDIR) $@; done

clean:
		rm -f *~ *.o *.a *.h $(PROG) retroImage gccdump.s
		for i in $(SUBDIR); do $(MAKE) -C $$i $(MFLAGS) clean; done

ar:             $(AR_OBJS)
		$(CC) $(LDFLAGS) -o $@ $(AR_OBJS) $(LIBS)

as:             as.o
		$(CC) $(LDFLAGS) -o $@ $< $(LIBS)

aout:           $(AOUT_OBJS)
		$(CC) $(LDFLAGS) -o $@ $(AOUT_OBJS) $(LIBS)

ld:             ld.o
		$(CC) $(LDFLAGS) -o $@ $< $(LIBS)

nm:             nm.o
		$(CC) $(LDFLAGS) -o $@ $< $(LIBS)

ranlib:         $(RANLIB_OBJS)
		$(CC) $(LDFLAGS) -o $@ $(RANLIB_OBJS) $(LIBS)

size:           size.o
		$(CC) $(LDFLAGS) -o $@ $< $(LIBS)

strip:          strip.o
		$(CC) $(LDFLAGS) -o $@ $< $(LIBS)

$(HEADERS):
		-ln -s -f ../include/a.out.h .
		-ln -s -f ../include/ar.h .
		-ln -s -f ../include/nlist.h .
		-ln -s -f ../include/ranlib.h .
