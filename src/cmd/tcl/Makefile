TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS		+= -Os -Wall -Werror -Wno-error=pointer-sign -Wno-pointer-sign

LDFLAGS		+=

OBJS		= tclsh.o
LIBS		= -ltcl

all:            tcl

tcl:         	${OBJS}
		${CC} ${LDFLAGS} -o tcl.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S tcl.elf > tcl.dis
		${SIZE} tcl.elf
		${ELF2AOUT} tcl.elf $@ && rm tcl.elf

clean:
		rm -f *.o *.0 *.elf tcl *.elf *.dis a.out tags *~

install:        all
		install tcl $(DESTDIR)/bin/

###
regexp.o: regexp.c regexp.h regpriv.h
regsub.o: regsub.c regexp.h regpriv.h
tclassem.o: tclassem.c internal.h tcl.h hash.h
tclbasic.o: tclbasic.c internal.h tcl.h hash.h
tclcmdah.o: tclcmdah.c internal.h tcl.h hash.h
tclcmdil.o: tclcmdil.c internal.h tcl.h hash.h
tclcmdmz.o: tclcmdmz.c internal.h tcl.h hash.h regexp.h regpriv.h
tclenv.o: tclenv.c internal.h tcl.h hash.h
tclexpr.o: tclexpr.c internal.h tcl.h hash.h
tclget.o: tclget.c internal.h tcl.h hash.h
tclglob.o: tclglob.c internal.h tcl.h hash.h
tclhash.o: tclhash.c internal.h tcl.h hash.h
tclparse.o: tclparse.c internal.h tcl.h hash.h
tclproc.o: tclproc.c internal.h tcl.h hash.h
tclsh.o: tclsh.c
tclunxaz.o: tclunxaz.c internal.h tcl.h hash.h
tclutil.o: tclutil.c internal.h tcl.h hash.h regexp.h
tcluxstr.o: tcluxstr.c internal.h tcl.h hash.h
tcluxutl.o: tcluxutl.c internal.h tcl.h hash.h
tclvar.o: tclvar.c internal.h tcl.h hash.h
