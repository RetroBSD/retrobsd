OBJS    = mkrd.o
BIN     = mkrd
CFLAGS  = -Wall -Werror -O

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $< $(LIBS) $(LDFLAGS)

clean:
	@rm -f $(BIN) $(OBJS)
