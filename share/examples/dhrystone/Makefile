OBJS    = dhry_1.o dhry_2.o

all:    dhrystone

dhrystone: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

clean:
	rm -f *.o dhrystone
