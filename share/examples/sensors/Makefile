PROG    = led2 led3 led7 relay buzzer laser pbuzz irled joystick

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

led7:   led7.c
	cc $? -o $@

irled:  irled.c
	cc $? -o $@

joystick: joystick.c
	cc $? -o $@

clean:
	rm -f *.o *~ $(PROG)
