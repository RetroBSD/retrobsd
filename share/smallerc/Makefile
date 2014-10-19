CC      = cc
PROG    = hello primelist primesum test1 test2 test3 gpio adc rain \
          q8

all:    $(PROG)

hello:	hello.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) hello.c

primelist: primelist.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) primelist.c

primesum: primesum.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) primesum.c

gpio:   gpio.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) gpio.c

adc:    adc.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) adc.c

rain:   rain.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) rain.c

q8:     q8.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) q8.c

test1:	test1.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) test1.c

test2:	test2.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) test2.c

test3:	test3.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) test3.c

clean:
	rm -f *.o $(PROG)
