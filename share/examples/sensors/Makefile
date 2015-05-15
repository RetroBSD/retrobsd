PROG    = led2 led3 relay buzzer laser pbuzz irled

all:    $(PROG)

relay:  relay.c
	cc $? -o $@

buzzer: buzzer.c
	cc $? -o $@

pbuzz: pbuzz.c
	cc $? -o $@

laser:  laser.c
	cc $? -o $@

led2:   led2.c
	cc $? -o $@

led3:   led3.c
	cc $? -o $@

irled:  irled.c
	cc $? -o $@

clean:
	rm -f *.o *~ $(PROG)
