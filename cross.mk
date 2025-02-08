DESTDIR		= /usr/local/retrobsd
MACHINE		= mips

CC		= gcc -m32

AS		= $(CC) -x assembler-with-cpp
LD		= ld
AR		= ar
RANLIB		= ranlib
YACC            = byacc
LEX             = flex
SIZE		= size
#OBJDUMP		= objdump
OBJDUMP		= sync --
INSTALL		= install -m 644
INSTALLDIR	= install -m 755 -d
TAGSFILE	= tags
MANROFF		= nroff -man -h
ELF2AOUT	= cp

CFLAGS		= -O -DCROSS
LDFLAGS		=
LIBS		=

# Add system include path
ifeq (/usr/include/x86_64-linux-gnu,$(wildcard /usr/include/x86_64-linux-gnu))
    CFLAGS      += -I/usr/include/x86_64-linux-gnu
else
ifeq (/usr/include/i386-linux-gnu,$(wildcard /usr/include/i386-linux-gnu))
    CFLAGS      += -I/usr/include/i386-linux-gnu
else
    CFLAGS      += -I/usr/include
endif
endif
CFLAGS		+= -I$(TOPSRC)/include
