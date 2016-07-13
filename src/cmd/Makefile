#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          += -Werror

# Programs that live in subdirectories, and have makefiles of their own.
# /bin
SUBDIR		= adb adc-demo aout ar as awk basic calendar cc chflags \
                  chpass cpp dc diff emg env fdisk find fold forth \
                  fstat glcdtest hostname id la lcc lcpp ld ls login make \
                  man md5 med more nm passwd pdc picoc portio printf pwm \
                  ranlib re renice retroforth scm sed setty sh sl smallc \
                  smlrc smux stty sysctl test uname wiznet xargs \
                  zmodem gtest msec cron compress date2 tip \
                  talloc uucp yacc

# /sbin
SUBDIR         += chown chroot disktool fsck getty init \
                  mkfs mknod mkpasswd mount pstat \
                  reboot shutdown umount update vipw virus

# TODO: ccom lex m4 yacc reloc sre pforth

# Shell scripts that need only be installed and are never removed.
#
SCRIPT		= false nohup true #lorder mkdep

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD		= basename cal cat cb chgrp chmod cmp col comm cp dd \
                  diskspeed du echo ed fgrep file grep head hostid join \
                  kill last ln mesg mkdir mv nice od pagesize pr printenv \
                  pwd rev rm rmail rmdir size sleep sort split sum sync \
                  tail tar tee time touch tr tsort tty uniq w wc whereis who

# C programs that live in the current directory and need explicit make lines.
#
NSTD		= bc expr egrep

# Programs that must run setuid to root
#
SETUID		= mail su

# Programs that run set-group-id operator
#
OPERATOR	= df

# Programs that must run set-group-id kmem.
#
KMEM		= ps iostat vmstat

# Programs that must run set-group-id tty.
#
TTY		= wall write

# 'strip' is handled specially because 'install -s' now uses 'strip' and
# thus we can't do a 'install -s strip /bin/strip' without an error.
#
BINS		= $(STD) $(NSTD) $(SETUID) $(OPERATOR) $(KMEM) $(TTY) $(SCRIPT) strip

all:		$(SUBDIR) $(BINS)

$(SUBDIR):	FRC
		$(MAKE) -C $@ $(MFLAGS)

FRC:

# $(STD) $(SETUID) $(KMEM) $(OPERATOR) $(TTY) strip
%: %.c
		$(CC) $(CFLAGS) $(LDFLAGS) -o $@.elf $< $(LIBS)
		$(OBJDUMP) -S $@.elf > $@.dis
		$(SIZE) $@.elf
		$(ELF2AOUT) $@.elf $@ && /bin/rm $@.elf

# Files listed in $(NSTD) have explicit make lines given below.

# bc egrep expr
%.c: %.y
		$(YACC) $(YFLAGS) $<
		-/bin/mv y.tab.c $@

$(SCRIPT):
		install -c $@.sh $@

install:	$(BINS)
		install $(STD) $(NSTD) strip $(DESTDIR)/bin/
		-for i in $(SUBDIR); do \
			$(MAKE) -C $$i $(MFLAGS) DESTDIR=$(DESTDIR) install; done
		-for i in $(SCRIPT); do install $$i.sh $(DESTDIR)/bin/$$i; done
		-for i in $(SETUID); do \
			install -m 4751 $$i $(DESTDIR)/bin/$$i; done
		-for i in $(OPERATOR); do \
			install -m 2751 $$i $(DESTDIR)/bin/$$i; done
		-for i in $(KMEM); do \
			install -m 2751 $$i $(DESTDIR)/bin/$$i; done
		-for i in $(TTY); do \
			install -m 2751 $$i $(DESTDIR)/bin/$$i; done

clean:
		/bin/rm -f $(BINS) expr.c a.out core *.s *.o *.dis *.elf *~ y.tab.c errs
		for i in $(SUBDIR); do (cd $$i; $(MAKE) $(MFLAGS) clean); done

depend:		expr.c
		for i in $(BINS); do \
		    cc -M $(INCPATH) $$i.c | sed 's/\.o//' | \
		    awk ' ( if ($$1 != prev) \
			( if (rec != "") print rec; rec = $$0; prev = $$1; ) \
			else ( if (length(rec $$2) > 78) ( print rec; rec = $$0; ) \
			else rec = rec " " $$2 ) ) \
			END ( print rec ) ' >> makedep; done
		echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
		echo '$$r makedep' >>eddep
		echo 'w' >>eddep
		cp Makefile Makefile.bak
		ed - Makefile < eddep
		/bin/rm eddep makedep
		echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
		echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
		echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it
