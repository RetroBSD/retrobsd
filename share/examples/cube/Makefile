PROG    = all backlight cube demo

# Duinomite board
#OBJS    = duinomite.o

# Fubarino board
OBJS    = fubarino.o

all:    $(PROG)

all:    all.c $(OBJS)
	cc $@.c $(OBJS) -o $@

backlight: backlight.c $(OBJS)
	cc $@.c $(OBJS) -o $@

cube:   cube.c $(OBJS)
	cc $@.c $(OBJS) -o $@

demo:   demo.c $(OBJS)
	cc $@.c $(OBJS) -o $@

clean:
	rm -f *.o *~ $(PROG)
