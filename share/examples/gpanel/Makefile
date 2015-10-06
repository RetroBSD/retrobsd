CC      = cc
PROG    = tft tftetris

all:    $(PROG)

clean:
	rm -f *.o *~ $(PROG)

tft:	tft.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $?

tftetris: tftetris.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $?
