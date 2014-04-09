TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

vpath %.c $(TOPSRC)/src/libcurses

CFLAGS		+= -B$(TOPSRC)/lib/ $(DEFS) -Wa,-x -Wall -Werror

OBJS		= addch.o addstr.o box.o clear.o clrtobot.o clrtoeol.o cr_put.o \
                  cr_tty.o curses.o delch.o deleteln.o delwin.o endwin.o erase.o \
                  fullname.o getch.o getstr.o idlok.o id_subwins.o initscr.o insch.o \
                  insertln.o longname.o move.o mvprintw.o mvscanw.o mvwin.o newwin.o \
                  overlay.o overwrite.o printw.o putchar.o refresh.o scanw.o scroll.o \
                  toucholap.o standout.o touchwin.o tstp.o unctrl.o

all:            ../libcurses.a

../libcurses.a: ../ar ../ranlib $(OBJS)
		../ar rc $@ $(OBJS)
		../ranlib $@

clean:
		rm -f *~ *.o a.out *.a
