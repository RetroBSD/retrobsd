
BIN     = config
SRCS    = main.cpp config.cpp mapping.cpp device.cpp cluster.cpp util.cpp core.cpp gstore.cpp
OBJS    = main.o config.o mapping.o device.o cluster.o util.o core.o gstore.o
CXXFLAGS = -Wall -O

all:    .depend $(BIN)

$(BIN): $(OBJS)
	g++ -o $@ $(OBJS) $(CXXFLAGS)

clean:
	-rm -f $(OBJS) $(BIN)

.depend: $(SRCS) Makefile
	g++ -MM $(CXXFLAGS) $(SRCS) > $@

-include .depend
