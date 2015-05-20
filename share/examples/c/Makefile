CC      = cc
PROG    = hello stdarg skeleton primelist primesum test1 test2 test3 \
          gpio adc rain q8 tetris lcd6

all:    $(PROG)

hello:	hello.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $?

stdarg: stdarg.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $?

skeleton: skeleton.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $?

primelist: primelist.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $?

primesum: primesum.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $?

gpio:   gpio.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $?

adc:    adc.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $?

rain:   rain.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $?

q8:     q8.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $?

tetris: tetris.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $?

lcd6:   lcd6.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $?

test1:	test1.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $?

test2:	test2.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $?

test3:	test3.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $?

clean:
	rm -f *.o *~ $(PROG)
