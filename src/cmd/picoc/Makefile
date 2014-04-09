TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

#CC=gcc
CFLAGS+= -g -DUNIX_HOST -DVER=\"`svnversion -n`\" -DFILENAME_MAX=64 -DL_tmpnam=30 -DCLOCKS_PER_SEC=80000000 -DPATH_MAX=200 -Os -Werror
LIBS=-lm -lc

TARGET	= picoc
SRCS	= picoc.c table.c lex.c parse.c expression.c heap.c type.c \
	variable.c clibrary.c platform.c include.c debug.c \
	platform/platform_unix.c platform/library_unix.c \
	cstdlib/stdio.c cstdlib/math.c cstdlib/string.c cstdlib/stdlib.c \
	cstdlib/time.c cstdlib/errno.c cstdlib/ctype.c cstdlib/stdbool.c \
	cstdlib/unistd.c retrobsd.c
OBJS	:= $(SRCS:%.c=%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	${CC} ${LDFLAGS} -o ${TARGET}.elf ${OBJS} ${LIBS}
	${OBJDUMP} -S ${TARGET}.elf > ${TARGET}.dis
	${SIZE} ${TARGET}.elf
	${ELF2AOUT} ${TARGET}.elf $@

install: $(TARGET)
	cp $(TARGET) $(TOPSRC)/bin

test:	all
	(cd tests; make test)

clean:
	rm -f $(TARGET) $(OBJS) *~ *.elf *.dis

count:
	@echo "Core:"
	@cat picoc.h interpreter.h picoc.c table.c lex.c parse.c expression.c platform.c heap.c type.c variable.c include.c debug.c | grep -v '^[ 	]*/\*' | grep -v '^[ 	]*$$' | wc
	@echo ""
	@echo "Everything:"
	@cat $(SRCS) *.h */*.h | wc

.PHONY: clibrary.c

picoc.o: picoc.c picoc.h
table.o: table.c interpreter.h platform.h
lex.o: lex.c interpreter.h platform.h
parse.o: parse.c picoc.h interpreter.h platform.h
expression.o: expression.c interpreter.h platform.h
heap.o: heap.c interpreter.h platform.h
type.o: type.c interpreter.h platform.h
variable.o: variable.c interpreter.h platform.h
clibrary.o: clibrary.c picoc.h interpreter.h platform.h
platform.o: platform.c picoc.h interpreter.h platform.h
include.o: include.c picoc.h interpreter.h platform.h
debug.o: debug.c interpreter.h platform.h
platform/platform_unix.o: platform/platform_unix.c picoc.h interpreter.h platform.h
platform/library_unix.o: platform/library_unix.c interpreter.h platform.h
cstdlib/stdio.o: cstdlib/stdio.c interpreter.h platform.h
cstdlib/math.o: cstdlib/math.c interpreter.h platform.h
cstdlib/string.o: cstdlib/string.c interpreter.h platform.h
cstdlib/stdlib.o: cstdlib/stdlib.c interpreter.h platform.h
cstdlib/time.o: cstdlib/time.c interpreter.h platform.h
cstdlib/errno.o: cstdlib/errno.c interpreter.h platform.h
cstdlib/ctype.o: cstdlib/ctype.c interpreter.h platform.h
cstdlib/stdbool.o: cstdlib/stdbool.c interpreter.h platform.h
cstdlib/unistd.o: cstdlib/unistd.c interpreter.h platform.h
