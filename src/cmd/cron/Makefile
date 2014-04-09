#/* Copyright 1988,1990,1993,1994 by Paul Vixie
# * All rights reserved
# *
# * Distribute freely, except: don't remove my name from the source or
# * documentation (don't take credit for my work), mark your changes (don't
# * get me blamed for your possible bugs), don't alter or remove this
# * notice.  May be sold if buildable source is provided to buyer.  No
# * warrantee of any kind, express or implied, is included with this
# * software; use at your own risk, responsibility for damages (if any) to
# * anyone resulting from the use of this software rests entirely with the
# * user.
# *
# * Send bug reports, bug fixes, enhancements, requests, flames, etc., and
# * I'll try to keep a version up to date.  I can be reached as follows:
# * Paul Vixie          <paul@vix.com>          uunet!decwrl!vixie!paul
# */

# Makefile for vixie's cron
#
# $Id: Makefile,v 2.9 1994/01/15 20:43:43 vixie Exp $
#
# vix 03mar88 [moved to RCS, rest of log is in there]
# vix 30mar87 [goodbye, time.c; hello, getopt]
# vix 12feb87 [cleanup for distribution]
# vix 30dec86 [written]

# NOTES:
#	'make' can be done by anyone
#	'make install' must be done by root
#
#	this package needs getopt(3), bitstring(3), and BSD install(8).
#
#	the configurable stuff in this makefile consists of compilation
#	options (use -O, cron runs forever) and destination directories.
#	SHELL is for the 'augumented make' systems where 'make' imports
#	SHELL from the environment and then uses it to run its commands.
#	if your environment SHELL variable is /bin/csh, make goes real
#	slow and sometimes does the wrong thing.  
#
#	this package needs the 'bitstring macros' library, which is
#	available from me or from the comp.sources.unix archive.  if you
#	put 'bitstring.h' in a non-standard place (i.e., not intuited by
#	cc(1)), you will have to define INCLUDE to set the include
#	directory for cc.  INCLUDE should be `-Isomethingorother'.
#
#	there's more configuration info in config.h; edit that first!

TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          += -Werror -I.


DESTROOT	=	$(DESTDIR)
DESTSBIN	=	$(DESTROOT)/sbin
DESTBIN		=	$(DESTROOT)/bin
DESTMAN		=	$(DESTROOT)/share/man
INCLUDE		=	-I.
OPTIM		=	-O
#<<lint flags of choice?>>
LINTFLAGS	=	-hbxa $(INCLUDE)
#CC		=	cc
DEFS		=
INSTALL 	= install
#LDFLAGS		= -i
#################################### end configurable stuff

SHELL		=	/bin/sh
#CFLAGS		=	$(OPTIM) $(INCLUDE) $(DEFS)

LINT_CRON	=	cron.c database.c user.c entry.c compat.c \
			misc.c job.c do_command.c env.c popen.c
LINT_CRONTAB	=	crontab.c misc.c entry.c env.c compat.c
CRON_OBJ	=	cron.o database.o user.o entry.o job.o do_command.o \
			misc.o env.o popen.o compat.o
CRONTAB_OBJ	=	crontab.o misc.o entry.o env.o compat.o

all:	cron crontab

lint:
	lint $(LINTFLAGS) $(LINT_CRON) $(LIBS) \
		|grep -v "constant argument to NOT" 2>&1
	lint $(LINTFLAGS) $(LINT_CRONTAB) $(LIBS) \
		|grep -v "constant argument to NOT" 2>&1

cron:	$(CRON_OBJ)
		${CC} ${LDFLAGS} -o $@.elf ${CRON_OBJ} ${LIBS}
		${OBJDUMP} -S $@.elf > $@.dis
		${SIZE} $@.elf
		${ELF2AOUT} $@.elf $@

crontab:	$(CRONTAB_OBJ)
		${CC} ${LDFLAGS} -o $@.elf ${CRONTAB_OBJ} ${LIBS}
		${OBJDUMP} -S $@.elf > $@.dis
		${SIZE} $@.elf
		${ELF2AOUT} $@.elf $@

install:	all
	cp cron $(DESTSBIN)/
	cp crontab $(DESTBIN)/
	sh putman.sh crontab.1 $(DESTMAN)
	sh putman.sh cron.8    $(DESTMAN)
	sh putman.sh crontab.5 $(DESTMAN)

clean:
	rm -f *.o cron crontab a.out core tags *~ *.elf *.dis

$(CRON_OBJ)	:	cron.h compat.h config.h externs.h pathnames.h Makefile
$(CRONTAB_OBJ)	:	cron.h compat.h config.h externs.h pathnames.h Makefile
