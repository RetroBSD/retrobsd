PROG    = flip jump timer typetext
#CFLAGS  = -DNCURSES_OPAQUE=0
LIBS    = -lcurses -ltermlib

all:    $(PROG)

clean:
	rm -f *.o *~ $(PROG)

flip:   flip.c
	cc $(CFLAGS) -o $@ $@.c $(LIBS)

jump:   jump.c
	cc $(CFLAGS) -o $@ $@.c $(LIBS)

timer:  timer.c
	cc $(CFLAGS) -o $@ $@.c $(LIBS)

typetext: typetext.c
	cc $(CFLAGS) -o $@ $@.c $(LIBS)
