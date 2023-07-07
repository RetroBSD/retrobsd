OBJS    = mkrd.o
BIN     = mkrd
CFLAGS  = -m32 -Wall -Werror -O 

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -m32 -o $@ $< $(LIBS) $(LDFLAGS)

clean:
	@rm -f $(BIN) $(OBJS)
