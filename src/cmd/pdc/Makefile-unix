#
# Makefile
#
# The programmers desktop calculator. A desktop calculator supporting both
# shifts and mixed base inputs.
#
# Copyright (C) 2001, 2002, 2003, 2004, 2005
#               Daniel Thompson <see help function for e-mail>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#

TARGET  = pdc

CC      = $(COMPILER_PREFIX)gcc
CFLAGS += -O2 -g

STRIP   = $(COMPILER_PREFIX)strip


ifndef WITHOUT_GCC
CFLAGS += -Wall
endif

ifdef WITH_CMDEDIT
CFLAGS += -DHAVE_CMDEDIT
else

# This assumes a dynamic link to libreadline.dll in .../bin. Defining
# WITHOUT_READLINE should have the normal effect. The .../lib part is just in
# case people do strange things to their layouts.
ifdef WIN32_HOME
CFLAGS += -I$(WIN32_HOME)/include -L$(WIN32_HOME)/lib -L$(WIN32_HOME)/bin
WITHOUT_NCURSES = 1
TARGET := $(TARGET).exe
endif

ifndef WITHOUT_READLINE
LDFLAGS += -lreadline
CFLAGS  += -DHAVE_READLINE
endif
ifndef WITHOUT_NCURSES
LDFLAGS += -lncurses
endif

endif

all: $(TARGET)

strip : $(TARGET)
	$(STRIP) $(TARGET)

$(TARGET) : y.tab.o
	$(CC) $(CFLAGS) -o $@ y.tab.o $(LDFLAGS)

y.tab.c : pdc.y
	$(YACC) pdc.y

clean :
	$(RM) $(TARGET) y.tab.c *.o
	
