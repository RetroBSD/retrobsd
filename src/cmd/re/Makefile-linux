#
# Visual text editor for Unix.
# Based on sources of RAND Editor.
# Part of Demos Operating System Project, Russia 1982-1991.
#
# Alex P. Roudnev, Moscow, KIAE, 1984
#
CFLAGS          = -Os -g -Wall -Werror -DDEBUG -DMULTIWIN=0

# For Linux and Mac OS X
CFLAGS          += -DTERMIOS

all:            re

OBJS            = r.cmd.o r.edit.o r.file.o r.gettc.o r.misc.o \
                  r.macro.o r.main.o r.display.o r.termcap.o r.ttyio.o r.window.o

re:             $(OBJS)
		cc $(OBJS) -o $@

$(OBJS):        r.defs.h

clean:
		rm -f *.o *~ re *.dis *.elf
