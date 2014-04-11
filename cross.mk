DESTDIR		= /usr/local/retrobsd
MACHINE		= mips

CC		= gcc

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

CFLAGS		= -O -DCROSS -I/usr/include -I$(TOPSRC)/include
LDFLAGS		=
LIBS		=
