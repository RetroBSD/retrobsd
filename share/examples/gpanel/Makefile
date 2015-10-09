CC      = cc
LIBS    = -lgpanel
PROG    = tft tftetris pixel line rect fill circle font

FONTS   = 5x7.o 6x9.o digits20.o digits32.o lucidasans11.o lucidasans15.o \
          lucidasans7.o lucidasans9.o verdana7.o

all:    $(PROG)

clean:
	rm -f *.o *~ $(PROG)

tft:	tft.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $?

tftetris: tftetris.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $?

pixel:  pixel.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $? $(LIBS)

line:   line.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $? $(LIBS)

rect:   rect.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $? $(LIBS)

fill:   fill.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $? $(LIBS)

circle: circle.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $? $(LIBS)

font:   font.c $(FONTS)
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) font.c $(FONTS) $(LIBS)

5x7.o:  fonts/5x7.c
	$(CC) $(CFLAGS) -o $@ -c $?

6x9.o:  fonts/6x9.c
	$(CC) $(CFLAGS) -o $@ -c $?

digits20.o: fonts/digits20.c
	$(CC) $(CFLAGS) -o $@ -c $?

digits32.o: fonts/digits32.c
	$(CC) $(CFLAGS) -o $@ -c $?

lucidasans11.o: fonts/lucidasans11.c
	$(CC) $(CFLAGS) -o $@ -c $?

lucidasans15.o: fonts/lucidasans15.c
	$(CC) $(CFLAGS) -o $@ -c $?

lucidasans7.o: fonts/lucidasans7.c
	$(CC) $(CFLAGS) -o $@ -c $?

lucidasans9.o: fonts/lucidasans9.c
	$(CC) $(CFLAGS) -o $@ -c $?

verdana7.o: fonts/verdana7.c
	$(CC) $(CFLAGS) -o $@ -c $?
